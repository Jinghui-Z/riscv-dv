#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/*
 * Mixed interrupt/breakpoint cross cases.
 *
 * STIP/SSIP CSR-pending cases are the current ordinary LinkNan/default
 * candidates. VModule/RNMI injection cases in this file are AP-IT/special-run
 * material only: keep them commented in test_register.c unless the selected
 * runner is confirmed VModule-capable and the whole ELF is built with
 * VMODULE=1. Do not use per-case #if VMODULE branches here; runner selection
 * owns that boundary.
 */

#define AI_IBC_MSTATUS_MPIE (1ULL << 7)
#define AI_IBC_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_IBC_MSTATUS_MPP_S (1ULL << 11)
#define AI_IBC_MSTATUS_MPP_M (3ULL << 11)
#define AI_IBC_MCONTROL6_MSU_BASE 0x6000000000000058ULL
#define AI_IBC_MCONTROL6_LOAD 0x1ULL
#define AI_IBC_MCONTROL6_STORE 0x2ULL
#define AI_IBC_MCONTROL6_EXECUTE 0x4ULL
#define AI_IBC_MCONTROL6_AMO (AI_IBC_MCONTROL6_LOAD | AI_IBC_MCONTROL6_STORE)
#define AI_IBC_MCONTROL6_U 0x8ULL
#define AI_IBC_MCONTROL6_S 0x10ULL
#define AI_IBC_MCONTROL6_M 0x40ULL
#define AI_IBC_MCONTROL6_MSU \
    (AI_IBC_MCONTROL6_M | AI_IBC_MCONTROL6_S | AI_IBC_MCONTROL6_U)
#define AI_IBC_MCONTROL6_CHAIN 0x800ULL
#define AI_IBC_MCONTROL6_ACTION_SHIFT 12U
#define AI_IBC_MCONTROL6_TYPE_SHIFT 60U
#define AI_IBC_MCONTROL6_TYPE_MCONTROL6 6ULL
#define AI_IBC_MIE_STIE (1ULL << 5)
#define AI_IBC_MIP_STIP (1ULL << 5)
#define AI_IBC_PMP_TOR_RWX 0x0fULL
#define AI_IBC_PMP_TOR_NO_W 0x0dULL
#define AI_IBC_SPIN_LIMIT 256ULL
#define AI_IBC_VMODULE_POLL_LIMIT 4096ULL
#define AI_IBC_STIP_WRITE_ROUNDS 8ULL
#define AI_IBC_LOG_SLOTS 8U
#define AI_IBC_TRIGGER_SLOTS 4U
#define AI_IBC_BAD_PADDR 0x1fffffffffff8ULL
#define AI_IBC_HANDLER_MODE_NONE 0ULL
#define AI_IBC_HANDLER_MODE_ENABLE_STIP_ON_BKP 1ULL
#define AI_IBC_HANDLER_MODE_SWITCH_MTVEC_ON_BKP 2ULL
#define AI_IBC_HANDLER_MODE_CLEAR_SLOT0_ON_STI 3ULL
#define AI_IBC_HANDLER_MODE_CLEAR_STIP_ON_BKP 4ULL
#define AI_IBC_HANDLER_MODE_COUNTER_LOG 5ULL
#define AI_IBC_HANDLER_MODE_SKIP_ILLEGAL 6ULL
#define AI_IBC_HANDLER_MODE_COUNTER_INHIBIT 7ULL
#define AI_IBC_HANDLER_MODE_CLEAR_SLOT0_ON_BKP 8ULL
#define AI_IBC_HANDLER_MODE_SKIP_LSQ_YOUNG_ON_BKP 9ULL
#define AI_IBC_HANDLER_MODE_MEI_OFF_BKP_ON_STATUS 10ULL
#define AI_IBC_HANDLER_MODE_LOG_VSTART 11ULL
#define AI_IBC_HANDLER_MODE_HS_BKP_OPEN_MEI_STACK 12ULL
#define AI_IBC_HANDLER_MODE_VECTOR_DIRTY_LOG 13ULL
#define AI_IBC_HANDLER_MODE_RNMI_DURING_MEI_CLEAR 14ULL
#define AI_IBC_HANDLER_MODE_SFENCE_REMAP_ON_BKP 15ULL
#define AI_IBC_HANDLER_MODE_REPAIR_PMP_ON_BKP 16ULL
#define AI_IBC_MCOUNTINHIBIT_CY_IR 0x5ULL
#define AI_IBC_MSTATUS_VS_MASK (3ULL << 9)
#define AI_IBC_MSTATUS_VS_CLEAN (2ULL << 9)
#define AI_IBC_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_IBC_NESTED_LOG_MAGIC_BASE 0x4942434e45535400ULL
#define AI_IBC_HS_LOG_MAGIC_BASE 0x4942434e45534800ULL
#define AI_IBC_VMODULE_REG_MAIN_INTERRUPT 0x1000ULL
#define AI_IBC_VMODULE_REG_INTERRUPT_DELAY 0x1028ULL
#define AI_IBC_VMODULE_REG_FORCE_NMI 0x1030ULL
#define AI_IBC_VMODULE_MAIN_MEI 0x00000800ULL
#define AI_IBC_VMODULE_NMI_31 0x1ULL
#define AI_IBC_VMODULE_ADDR_CMD_PREFIX 0xffff000100000000ULL
#define AI_IBC_VMODULE_DATA_CMD_PREFIX 0xffff000000000000ULL
#define AI_IBC_NMI31_CAUSE (CAUSE_INT_BIT | 31ULL)
#define AI_IBC_SCRATCH_INITIAL_MS 0x6d736372696e6901ULL
#define AI_IBC_SCRATCH_INITIAL_MN 0x6e736372696e6901ULL
#define AI_IBC_SCRATCH_BP_MS 0x6d73637262700001ULL
#define AI_IBC_SCRATCH_MEI_MS 0x6d7363726d656901ULL
#define AI_IBC_SCRATCH_RNMI_MN 0x6e736372726e0101ULL

#ifndef CSR_MNSCRATCH
#define CSR_MNSCRATCH 0x740
#endif

extern void ai_ibc_ebreak_probe(void);
extern void ai_ibc_cebreak_probe(void);
extern void ai_ibc_rnmi_ebreak_marker_probe(volatile uint64_t *marker);
extern void ai_ibc_rnmi_ebreak_inject_site(void);
extern void ai_ibc_rnmi_ebreak_marker_site(void);
extern void ai_ibc_rnmi_cebreak_marker_probe(volatile uint64_t *marker);
extern void ai_ibc_rnmi_cebreak_inject_site(void);
extern void ai_ibc_rnmi_cebreak_marker_site(void);
extern uint64_t ai_ibc_load_once(uintptr_t addr);
extern void ai_ibc_load_dependent_store_probe(uint64_t *load_addr,
    uint64_t *marker_addr,
    uint64_t sentinel);
extern void ai_ibc_load_dependent_store_load_site(void);
extern void ai_ibc_store_once(uintptr_t addr, uint64_t value);
extern void ai_ibc_cbozero_once(uintptr_t addr);
extern void ai_ibc_cbozero_site(void);
extern void ai_ibc_prefetch_i_once(uintptr_t addr);
extern void ai_ibc_chain_exec_store_probe(uint64_t *addr, uint64_t value);
extern void ai_ibc_chain_exec_store_store_site(void);
extern uint64_t ai_ibc_lr_d_once(uintptr_t addr);
extern uint64_t ai_ibc_sc_d_status_probe(uint64_t *addr,
    uint64_t value,
    uint64_t sentinel);
extern void ai_ibc_sc_d_status_site(void);
extern uint64_t ai_ibc_amo_aqrl_order_probe(uint64_t *amo_addr,
    uint64_t *before_addr,
    uint64_t *after_addr,
    uint64_t before_payload,
    uint64_t addend,
    uint64_t sentinel);
extern void ai_ibc_amo_aqrl_order_site(void);
extern void ai_ibc_lsq_store_pair_probe(uint64_t *older_path,
    uint64_t *younger_path,
    uint64_t older_payload,
    uint64_t younger_payload);
extern void ai_ibc_lsq_store_pair_older_site(void);
extern void ai_ibc_lsq_store_pair_after_young_site(void);
extern void ai_ibc_vector_store64_probe(uint64_t *src, uint64_t *dst);
extern void ai_ibc_vector_store64_site(void);
extern void ai_ibc_vector_store64_current_probe(uint64_t *src, uint64_t *dst);
extern void ai_ibc_vector_store64_current_site(void);
extern void ai_ibc_vector_load64_dirty_probe(uint64_t *src,
    uint64_t *dst,
    uint64_t seed);
extern void ai_ibc_vector_load64_dirty_site(void);
extern void ai_ibc_vector_load64_dirty_after_site(void);
extern void ai_ibc_mret_to_ebreak_probe(uint64_t *marker);
extern void ai_ibc_mret_ebreak_target(void);
extern void ai_ibc_wfi_then_ebreak_probe(uint64_t *marker);
extern void ai_ibc_wfi_ebreak_target(void);
extern void ai_ibc_wfi_then_store_probe(uint64_t *target,
    uint64_t payload,
    volatile uint64_t *marker,
    uint64_t marker_delta);
extern void ai_ibc_wfi_store_site(void);
extern void ai_ibc_branch_redirect_probe(uint64_t *wrong_path,
    uint64_t *right_path,
    uint64_t take_branch);
extern void ai_ibc_branch_wrong_path_site(void);
extern void ai_ibc_branch_right_path_site(void);
extern void ai_ibc_indirect_predict_probe(uint64_t *wrong_path,
    uint64_t *right_path,
    uintptr_t target);
extern void ai_ibc_indirect_wrong_target(void);
extern void ai_ibc_indirect_wrong_store_site(void);
extern void ai_ibc_indirect_right_target(void);
extern void ai_ibc_indirect_right_store_site(void);
extern void ai_ibc_icache_refill_cross_probe(volatile uint64_t *old_marker,
    volatile uint64_t *right_marker,
    uint64_t take_right,
    uint64_t old_payload,
    uint64_t right_payload);
extern void ai_ibc_icache_refill_old_block_site(void);
extern void ai_ibc_icache_refill_right_block_site(void);
extern void ai_ibc_mtvec_log_handler(void);
extern void ai_ibc_stvec_log_ssip_handler(void);
extern void ai_ibc_rnmi_bkp_csr_owner_handler(void);
extern void ai_ibc_masked_rnmi_ordinary_snapshot_handler(void);
extern void ai_ibc_rnmi_scratch_isolation_handler(void);
extern void ai_ibc_rnmi_source_clear_order_handler(void);
extern void ai_ibc_rnmi_mtvec_switch_handler(void);
extern void ai_ibc_rnmi_mtvec_switch_alt_mei_handler(void);
extern void ai_ibc_unexpected_rnmi_bkp_fault_handler(void);
extern void ai_ibc_stip_handler_ebreak_probe(void);
extern void ai_ibc_nested_handler_ebreak_site(void);
extern void ai_ibc_ebreak_illegal32_probe(volatile uint64_t *marker);
extern void ai_ibc_ebreak_illegal32_site(void);
extern void ai_ibc_illegal32_site(void);
extern void ai_ibc_ebreak_cillegal16_probe(volatile uint64_t *marker);
extern void ai_ibc_ebreak_cillegal16_site(void);
extern void ai_ibc_cillegal16_site(void);
extern void ai_ibc_rvc_rvi_mixed_width_probe(volatile uint64_t *marker);
extern void ai_ibc_mixed_cebreak_site(void);
extern void ai_ibc_mixed_ebreak_site(void);
extern void ai_ibc_frontend_lane_mixed_probe(volatile uint64_t *wrong_marker,
    volatile uint64_t *trigger_marker,
    volatile uint64_t *cebreak_marker,
    volatile uint64_t *ebreak_marker,
    uint64_t take_right_path);
extern void ai_ibc_frontend_lane_wrong_trigger_site(void);
extern void ai_ibc_frontend_lane_right_trigger_site(void);
extern void ai_ibc_frontend_lane_cebreak_site(void);
extern void ai_ibc_frontend_lane_ebreak_site(void);
extern void ai_ibc_trigger_then_illegal32_probe(volatile uint64_t *trigger_marker,
    volatile uint64_t *after_marker);
extern void ai_ibc_lane_trigger32_site(void);
extern void ai_ibc_lane_illegal32_site(void);
extern void ai_ibc_cillegal16_then_trigger_probe(volatile uint64_t *illegal_marker,
    volatile uint64_t *trigger_marker,
    volatile uint64_t *after_marker);
extern void ai_ibc_lane_cillegal16_site(void);
extern void ai_ibc_lane_after_cillegal16_site(void);
extern void ai_ibc_lane_trigger_after_cillegal_site(void);
extern void ai_ibc_mtvec_vectored_table(void);
extern void ai_ibc_mtvec_new_vectored_table(void);
extern uint64_t ai_ibc_csrrs_mie_stie(void);
extern uint64_t ai_ibc_csrrc_mie_stie(void);

static uint64_t ai_ibc_mem[4] __attribute__((aligned(16)));
static volatile uint64_t ai_ibc_trap_count;
static volatile uint64_t ai_ibc_trap_cause[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_epc[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_tval[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_vstart[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_mip[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_magic[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_mstatus[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_mncause[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_mnepc[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_mnstatus[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_pmpcfg0[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_pmpaddr0[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_trap_pmpaddr1[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_rnmi_hits;
static volatile uint64_t ai_ibc_rnmi_mncause[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_rnmi_mnepc[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_rnmi_mnstatus[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_rnmi_mnret_markers;
static volatile uint64_t ai_ibc_rnmi_mret_markers;
static volatile uint64_t ai_ibc_handler_mode;
static volatile uint64_t ai_ibc_nested_after_marker;
static volatile uint64_t ai_ibc_nested_outer_epc;
static volatile uint64_t ai_ibc_nested_outer_mstatus;
static volatile uint64_t ai_ibc_vectored_direct_hits;
static volatile uint64_t ai_ibc_vectored_stip_hits;
static volatile uint64_t ai_ibc_vectored_unexpected_hits;
static volatile uint64_t ai_ibc_new_vectored_stip_hits;
static volatile uint64_t ai_ibc_interrupt_cleared_trigger_hits;
static volatile uint64_t ai_ibc_breakpoint_cleared_trigger_hits;
static volatile uint64_t ai_ibc_counter_cycle[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_counter_instret[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_counter_inhibit_before[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_counter_inhibit_after[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_counter_inhibit_restored[AI_IBC_LOG_SLOTS];
static volatile uint64_t ai_ibc_scratch_bp_hits;
static volatile uint64_t ai_ibc_scratch_mei_hits;
static volatile uint64_t ai_ibc_scratch_unexpected_hits;
static volatile uint64_t ai_ibc_scratch_bp_mscratch_before;
static volatile uint64_t ai_ibc_scratch_bp_mscratch_after_rnmi;
static volatile uint64_t ai_ibc_scratch_bp_mnscratch_after_rnmi;
static volatile uint64_t ai_ibc_scratch_rnmi_mscratch_seen;
static volatile uint64_t ai_ibc_scratch_rnmi_mnscratch_before;
static volatile uint64_t ai_ibc_scratch_rnmi_mnscratch_written;
static volatile uint64_t ai_ibc_scratch_mei_mscratch_before;
static volatile uint64_t ai_ibc_scratch_mei_mnscratch_seen;
static volatile uint64_t ai_ibc_source_order_bp_hits;
static volatile uint64_t ai_ibc_source_order_mei_hits;
static volatile uint64_t ai_ibc_source_order_unexpected_hits;
static volatile uint64_t ai_ibc_source_order_bp_mip_before_rnmi;
static volatile uint64_t ai_ibc_source_order_bp_mip_after_rnmi;
static volatile uint64_t ai_ibc_source_order_bp_mstatus_after_rnmi;
static volatile uint64_t ai_ibc_source_order_bp_mie_after_rnmi;
static volatile uint64_t ai_ibc_source_order_mei_mip_before_clear;
static volatile uint64_t ai_ibc_source_order_mei_mip_after_clear;
static volatile uint64_t ai_ibc_rnmi_switch_bp_hits;
static volatile uint64_t ai_ibc_rnmi_switch_alt_mei_hits;
static volatile uint64_t ai_ibc_rnmi_switch_unexpected_hits;
static volatile uint64_t ai_ibc_rnmi_switch_rnmi_entry_id;
static volatile uint64_t ai_ibc_rnmi_switch_mtvec_after_bkp;
static volatile uint64_t ai_ibc_rnmi_switch_alt_mtvec_seen;
static volatile uint64_t ai_ibc_unexpected_rnmi_bkp_hits;
static volatile uint64_t ai_ibc_unexpected_rnmi_fault_hits;
static volatile uint64_t ai_ibc_unexpected_rnmi_after_fault_marker;
static volatile uint64_t ai_ibc_unexpected_rnmi_unexpected_hits;
static volatile uint64_t ai_ibc_priv_stack_outer_mepc;
static volatile uint64_t ai_ibc_priv_stack_outer_mstatus;
static volatile uint64_t ai_ibc_priv_stack_after_mei_mstatus;
static volatile uint64_t ai_ibc_priv_stack_final_mstatus;
static volatile uint64_t ai_ibc_priv_stack_bp_returns;
static volatile uint64_t ai_ibc_priv_stack_mei_returns;
static volatile uint64_t ai_ibc_vector_dirty_mstatus_before;
static volatile uint64_t ai_ibc_vector_dirty_mstatus_after;
static volatile uint64_t ai_ibc_vector_dirty_mei_mstatus;
static volatile uint64_t ai_ibc_pmp_repair_addr0;
static volatile uint64_t ai_ibc_pmp_repair_addr1;
static volatile uint64_t ai_ibc_pmp_repair_addr2;
static volatile uint64_t ai_ibc_pmp_repair_cfg;
static volatile uint64_t ai_ibc_pmp_repair_hits;
static volatile uint64_t ai_ibc_sfence_target_pte;
static volatile uint64_t ai_ibc_sfence_new_pte;
static volatile uint64_t ai_ibc_sfence_handler_hits;
static pte_t ai_ibc_satp_root_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l1_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l2_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l3_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_root_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l1_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l2_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_ibc_satp_l3_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static uint64_t ai_ibc_vec_src[4] __attribute__((aligned(16)));
static uint64_t ai_ibc_vec_dst[4] __attribute__((aligned(16)));
static uint32_t ai_ibc_smc_code[4] __attribute__((aligned(16)));

static inline void ai_ibc_clear_mcontrol6_slot(size_t slot);
static inline void ai_ibc_clear_all_mcontrol6_slots(void);
static inline void ai_ibc_arm_mcontrol6_slot_priv(size_t slot,
    uintptr_t addr,
    uint64_t access_bits,
    uint64_t priv_bits,
    bool chain);
static inline uint64_t ai_ibc_read_trigger_csr_slot(size_t slot,
    uint64_t *tdata1,
    uint64_t *tdata2);

asm(
    ".section .text\n"
    ".align 2\n"
    ".global ai_ibc_ebreak_probe\n"
    "ai_ibc_ebreak_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  ebreak\n"
    "  ret\n"
    "  .option pop\n"
    ".align 1\n"
    ".global ai_ibc_cebreak_probe\n"
    "ai_ibc_cebreak_probe:\n"
    "  .2byte 0x9002\n"
    "  ret\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_ebreak_marker_probe\n"
    ".global ai_ibc_rnmi_ebreak_inject_site\n"
    ".global ai_ibc_rnmi_ebreak_marker_site\n"
    "ai_ibc_rnmi_ebreak_marker_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "ai_ibc_rnmi_ebreak_inject_site:\n"
    "  li t0, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  li t0, 0xffff000000000001\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  la t2, ai_ibc_rnmi_hits\n"
    "  li t3, 4096\n"
    ".Lai_ibc_rnmi_ebreak_wait:\n"
    "  ld t4, 0(t2)\n"
    "  bnez t4, .Lai_ibc_rnmi_ebreak_wait_done\n"
    "  addi t3, t3, -1\n"
    "  bnez t3, .Lai_ibc_rnmi_ebreak_wait\n"
    "  li t1, 0x726e6d69746f41\n"
    "  sd t1, 0(a0)\n"
    ".Lai_ibc_rnmi_ebreak_wait_done:\n"
    "ai_ibc_rnmi_ebreak_marker_site:\n"
    "  ebreak\n"
    "  ld t1, 0(a0)\n"
    "  addi t1, t1, 0x41\n"
    "  sd t1, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 1\n"
    ".global ai_ibc_rnmi_cebreak_marker_probe\n"
    ".global ai_ibc_rnmi_cebreak_inject_site\n"
    ".global ai_ibc_rnmi_cebreak_marker_site\n"
    "ai_ibc_rnmi_cebreak_marker_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "ai_ibc_rnmi_cebreak_inject_site:\n"
    "  li t0, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  li t0, 0xffff000000000001\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  la t2, ai_ibc_rnmi_hits\n"
    "  li t3, 4096\n"
    ".Lai_ibc_rnmi_cebreak_wait:\n"
    "  ld t4, 0(t2)\n"
    "  bnez t4, .Lai_ibc_rnmi_cebreak_wait_done\n"
    "  addi t3, t3, -1\n"
    "  bnez t3, .Lai_ibc_rnmi_cebreak_wait\n"
    "  li t1, 0x726e6d69746f23\n"
    "  sd t1, 0(a0)\n"
    ".Lai_ibc_rnmi_cebreak_wait_done:\n"
    "  .option rvc\n"
    "ai_ibc_rnmi_cebreak_marker_site:\n"
    "  .2byte 0x9002\n"
    "  .option norvc\n"
    "  ld t1, 0(a0)\n"
    "  addi t1, t1, 0x23\n"
    "  sd t1, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_load_once\n"
    "ai_ibc_load_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  ld a0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_load_dependent_store_probe\n"
    ".global ai_ibc_load_dependent_store_load_site\n"
    "ai_ibc_load_dependent_store_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  mv t0, a2\n"
    "ai_ibc_load_dependent_store_load_site:\n"
    "  ld t0, 0(a0)\n"
    "  sd t0, 0(a1)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_store_once\n"
    "ai_ibc_store_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  sd a1, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_cbozero_once\n"
    ".global ai_ibc_cbozero_site\n"
    "ai_ibc_cbozero_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "ai_ibc_cbozero_site:\n"
    "  .insn i 0x0f, 0x2, x0, a0, 4\n"
    "  fence rw, rw\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_prefetch_i_once\n"
    "ai_ibc_prefetch_i_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  .insn i 0x13, 0x6, x0, a0, 0\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_chain_exec_store_probe\n"
    ".global ai_ibc_chain_exec_store_store_site\n"
    "ai_ibc_chain_exec_store_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  mv t0, a1\n"
    "ai_ibc_chain_exec_store_store_site:\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_lr_d_once\n"
    "ai_ibc_lr_d_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  lr.d a0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_sc_d_status_probe\n"
    ".global ai_ibc_sc_d_status_site\n"
    "ai_ibc_sc_d_status_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  mv t0, a2\n"
    "ai_ibc_sc_d_status_site:\n"
    "  sc.d t0, a1, 0(a0)\n"
    "  mv a0, t0\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_amo_aqrl_order_probe\n"
    ".global ai_ibc_amo_aqrl_order_site\n"
    "ai_ibc_amo_aqrl_order_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  mv t0, a5\n"
    "  sd a3, 0(a1)\n"
    "ai_ibc_amo_aqrl_order_site:\n"
    "  amoadd.d.aqrl t0, a4, (a0)\n"
    "  sd t0, 0(a2)\n"
    "  mv a0, t0\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_lsq_store_pair_probe\n"
    ".global ai_ibc_lsq_store_pair_older_site\n"
    ".global ai_ibc_lsq_store_pair_after_young_site\n"
    "ai_ibc_lsq_store_pair_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "ai_ibc_lsq_store_pair_older_site:\n"
    "  sd a2, 0(a0)\n"
    "  sd a3, 0(a1)\n"
    "ai_ibc_lsq_store_pair_after_young_site:\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_vector_store64_probe\n"
    ".global ai_ibc_vector_store64_site\n"
    "ai_ibc_vector_store64_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  li t0, 4\n"
    "  vsetvli t0, t0, e64, m1, ta, ma\n"
    "  csrw vstart, zero\n"
    "  vle64.v v6, (a0)\n"
    "  csrw vstart, zero\n"
    "ai_ibc_vector_store64_site:\n"
    "  vse64.v v6, (a1)\n"
    "  fence rw, rw\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_vector_store64_current_probe\n"
    ".global ai_ibc_vector_store64_current_site\n"
    "ai_ibc_vector_store64_current_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  csrw vstart, zero\n"
    "  vle64.v v6, (a0)\n"
    "  csrw vstart, zero\n"
    "ai_ibc_vector_store64_current_site:\n"
    "  vse64.v v6, (a1)\n"
    "  fence rw, rw\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_vector_load64_dirty_probe\n"
    ".global ai_ibc_vector_load64_dirty_site\n"
    ".global ai_ibc_vector_load64_dirty_after_site\n"
    "ai_ibc_vector_load64_dirty_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  sd a2, 0(a1)\n"
    "  csrr t2, mstatus\n"
    "  la t3, ai_ibc_vector_dirty_mstatus_before\n"
    "  sd t2, 0(t3)\n"
    "ai_ibc_vector_load64_dirty_site:\n"
    "  vle64.v v6, (a0)\n"
    "  csrw vstart, zero\n"
    "  vse64.v v6, (a1)\n"
    "  csrr t2, mstatus\n"
    "  la t3, ai_ibc_vector_dirty_mstatus_after\n"
    "  sd t2, 0(t3)\n"
    "  fence rw, rw\n"
    "ai_ibc_vector_load64_dirty_after_site:\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_branch_redirect_probe\n"
    ".global ai_ibc_branch_wrong_path_site\n"
    ".global ai_ibc_branch_right_path_site\n"
    "ai_ibc_branch_redirect_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  bnez a2, 1f\n"
    "  li t1, 0x77726f6e67\n"
    "ai_ibc_branch_wrong_path_site:\n"
    "  sd t1, 0(a0)\n"
    "  ret\n"
    "1:\n"
    "  li t1, 0x7269676874\n"
    "ai_ibc_branch_right_path_site:\n"
    "  sd t1, 0(a1)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_indirect_predict_probe\n"
    "ai_ibc_indirect_predict_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -16\n"
    "  sd ra, 0(sp)\n"
    "  jalr ra, a2, 0\n"
    "  ld ra, 0(sp)\n"
    "  addi sp, sp, 16\n"
    "  ret\n"
    ".global ai_ibc_indirect_wrong_target\n"
    "ai_ibc_indirect_wrong_target:\n"
    "  li t1, 0x696e646972777201\n"
    ".global ai_ibc_indirect_wrong_store_site\n"
    "ai_ibc_indirect_wrong_store_site:\n"
    "  sd t1, 0(a0)\n"
    "  ret\n"
    ".global ai_ibc_indirect_right_target\n"
    "ai_ibc_indirect_right_target:\n"
    "  li t1, 0x696e646972727401\n"
    ".global ai_ibc_indirect_right_store_site\n"
    "ai_ibc_indirect_right_store_site:\n"
    "  sd t1, 0(a1)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_icache_refill_cross_probe\n"
    ".global ai_ibc_icache_refill_old_block_site\n"
    ".global ai_ibc_icache_refill_right_block_site\n"
    "ai_ibc_icache_refill_cross_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  fence.i\n"
    "  bnez a2, .Lai_ibc_icache_refill_right_block\n"
    "  .align 6\n"
    "ai_ibc_icache_refill_old_block_site:\n"
    "  sd a3, 0(a0)\n"
    "  ret\n"
    "  .rept 24\n"
    "  nop\n"
    "  .endr\n"
    "  .align 6\n"
    ".Lai_ibc_icache_refill_right_block:\n"
    "ai_ibc_icache_refill_right_block_site:\n"
    "  sd a4, 0(a1)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_csrrs_mie_stie\n"
    "ai_ibc_csrrs_mie_stie:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  li t0, 32\n"
    "  csrrs a0, mie, t0\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_csrrc_mie_stie\n"
    "ai_ibc_csrrc_mie_stie:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  li t0, 32\n"
    "  csrrc a0, mie, t0\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_ebreak_illegal32_probe\n"
    ".global ai_ibc_ebreak_illegal32_site\n"
    "ai_ibc_ebreak_illegal32_probe:\n"
    "ai_ibc_ebreak_illegal32_site:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  ebreak\n"
    ".global ai_ibc_illegal32_site\n"
    "ai_ibc_illegal32_site:\n"
    "  .word 0xffffffff\n"
    "  li t0, 0x693332\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_ebreak_cillegal16_probe\n"
    ".global ai_ibc_ebreak_cillegal16_site\n"
    "ai_ibc_ebreak_cillegal16_probe:\n"
    "ai_ibc_ebreak_cillegal16_site:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  ebreak\n"
    ".global ai_ibc_cillegal16_site\n"
    "ai_ibc_cillegal16_site:\n"
    "  .2byte 0x0000\n"
    "  li t0, 0x693136\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_rvc_rvi_mixed_width_probe\n"
    "ai_ibc_rvc_rvi_mixed_width_probe:\n"
    "  .option push\n"
    "  .option rvc\n"
    ".global ai_ibc_mixed_cebreak_site\n"
    "ai_ibc_mixed_cebreak_site:\n"
    "  .2byte 0x9002\n"
    "  ld t0, 0(a0)\n"
    "  addi t0, t0, 0x11\n"
    "  sd t0, 0(a0)\n"
    "  .option norvc\n"
    ".global ai_ibc_mixed_ebreak_site\n"
    "ai_ibc_mixed_ebreak_site:\n"
    "  ebreak\n"
    "  ld t0, 0(a0)\n"
    "  addi t0, t0, 0x22\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 5\n"
    ".global ai_ibc_frontend_lane_mixed_probe\n"
    "ai_ibc_frontend_lane_mixed_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  bnez a4, .Lai_ibc_frontend_lane_right_path\n"
    ".global ai_ibc_frontend_lane_wrong_trigger_site\n"
    "ai_ibc_frontend_lane_wrong_trigger_site:\n"
    "  sd a4, 0(a0)\n"
    "  ret\n"
    ".Lai_ibc_frontend_lane_right_path:\n"
    ".global ai_ibc_frontend_lane_right_trigger_site\n"
    "ai_ibc_frontend_lane_right_trigger_site:\n"
    "  sd a4, 0(a1)\n"
    ".global ai_ibc_frontend_lane_cebreak_site\n"
    "ai_ibc_frontend_lane_cebreak_site:\n"
    "  .2byte 0x9002\n"
    "  ld t0, 0(a2)\n"
    "  addi t0, t0, 0x16\n"
    "  sd t0, 0(a2)\n"
    ".global ai_ibc_frontend_lane_ebreak_site\n"
    "ai_ibc_frontend_lane_ebreak_site:\n"
    "  ebreak\n"
    "  ld t0, 0(a3)\n"
    "  addi t0, t0, 0x32\n"
    "  sd t0, 0(a3)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_trigger_then_illegal32_probe\n"
    "ai_ibc_trigger_then_illegal32_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  li t0, 0x747269673332\n"
    ".global ai_ibc_lane_trigger32_site\n"
    "ai_ibc_lane_trigger32_site:\n"
    "  sd t0, 0(a0)\n"
    ".global ai_ibc_lane_illegal32_site\n"
    "ai_ibc_lane_illegal32_site:\n"
    "  .word 0xffffffff\n"
    "  li t0, 0x696c673332\n"
    "  sd t0, 0(a1)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_cillegal16_then_trigger_probe\n"
    "ai_ibc_cillegal16_then_trigger_probe:\n"
    "  .option push\n"
    "  .option rvc\n"
    ".global ai_ibc_lane_cillegal16_site\n"
    "ai_ibc_lane_cillegal16_site:\n"
    "  .2byte 0x0000\n"
    ".global ai_ibc_lane_after_cillegal16_site\n"
    "  .option norvc\n"
    "ai_ibc_lane_after_cillegal16_site:\n"
    "  li t0, 0x696c673136\n"
    "  sd t0, 0(a0)\n"
    "  li t0, 0x747269673136\n"
    ".global ai_ibc_lane_trigger_after_cillegal_site\n"
    "ai_ibc_lane_trigger_after_cillegal_site:\n"
    "  sd t0, 0(a1)\n"
    "  li t0, 0x6166743136\n"
    "  sd t0, 0(a2)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_mret_to_ebreak_probe\n"
    "ai_ibc_mret_to_ebreak_probe:\n"
    "  la t0, ai_ibc_mret_ebreak_target\n"
    "  csrw mepc, t0\n"
    "  mret\n"
    ".align 2\n"
    ".global ai_ibc_mret_ebreak_target\n"
    "ai_ibc_mret_ebreak_target:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  ebreak\n"
    "  li t0, 0x6d726574\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_wfi_then_ebreak_probe\n"
    "ai_ibc_wfi_then_ebreak_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  wfi\n"
    ".global ai_ibc_wfi_ebreak_target\n"
    "ai_ibc_wfi_ebreak_target:\n"
    "  ebreak\n"
    "  li t0, 0x776669\n"
    "  sd t0, 0(a0)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_wfi_then_store_probe\n"
    ".global ai_ibc_wfi_store_site\n"
    "ai_ibc_wfi_then_store_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  wfi\n"
    "ai_ibc_wfi_store_site:\n"
    "  sd a1, 0(a0)\n"
    "  ld t0, 0(a2)\n"
    "  add t0, t0, a3\n"
    "  sd t0, 0(a2)\n"
    "  ret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_mtvec_log_handler\n"
    "ai_ibc_mtvec_log_handler:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  beqz t2, .Lai_ibc_alt_mei_rnmi_record\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  li t3, 8\n"
    "  beq t0, t3, .Lai_ibc_log_handler_framework_ecall\n"
    "  li t3, 9\n"
    "  beq t0, t3, .Lai_ibc_log_handler_framework_ecall\n"
    "  li t3, 10\n"
    "  beq t0, t3, .Lai_ibc_log_handler_framework_ecall\n"
    "  li t3, 11\n"
    "  beq t0, t3, .Lai_ibc_log_handler_framework_ecall\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x4942434e45535400\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, pmpcfg0\n"
    "  la t6, ai_ibc_trap_pmpcfg0\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, pmpaddr0\n"
    "  la t6, ai_ibc_trap_pmpaddr0\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, pmpaddr1\n"
    "  la t6, ai_ibc_trap_pmpaddr1\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  la t3, ai_ibc_handler_mode\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 5\n"
    "  beq t4, t5, .Lai_ibc_log_handler_do_counter_log\n"
    "  li t5, 7\n"
    "  bne t4, t5, .Lai_ibc_log_handler_skip_counter_log\n"
    ".Lai_ibc_log_handler_do_counter_log:\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, .Lai_ibc_log_handler_skip_counter_log\n"
    "  li t5, 8\n"
    "  bgtu t4, t5, .Lai_ibc_log_handler_skip_counter_log\n"
    "  addi t4, t4, -1\n"
    "  slli t5, t4, 3\n"
    "  csrr t6, mcycle\n"
    "  la t3, ai_ibc_counter_cycle\n"
    "  add t3, t3, t5\n"
    "  sd t6, 0(t3)\n"
    "  csrr t6, minstret\n"
    "  la t3, ai_ibc_counter_instret\n"
    "  add t3, t3, t5\n"
    "  sd t6, 0(t3)\n"
    "  la t3, ai_ibc_handler_mode\n"
    "  ld t4, 0(t3)\n"
    "  li t6, 7\n"
    "  bne t4, t6, .Lai_ibc_log_handler_skip_counter_log\n"
    "  csrr t2, mcountinhibit\n"
    "  la t3, ai_ibc_counter_inhibit_before\n"
    "  add t3, t3, t5\n"
    "  sd t2, 0(t3)\n"
    "  li t6, 5\n"
    "  csrs mcountinhibit, t6\n"
    "  csrr t6, mcountinhibit\n"
    "  la t3, ai_ibc_counter_inhibit_after\n"
    "  add t3, t3, t5\n"
    "  sd t6, 0(t3)\n"
    "  csrw mcountinhibit, t2\n"
    "  csrr t6, mcountinhibit\n"
    "  la t3, ai_ibc_counter_inhibit_restored\n"
    "  add t3, t3, t5\n"
    "  sd t6, 0(t3)\n"
    ".Lai_ibc_log_handler_skip_counter_log:\n"
    "  li t3, 3\n"
    "  bne t0, t3, 3f\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, 4f\n"
    "  addi t1, t1, 4\n"
    "  j 5f\n"
    "4:\n"
    "  addi t1, t1, 2\n"
    "5:\n"
    "  csrw mepc, t1\n"
    "  la t3, ai_ibc_handler_mode\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 1\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_switch_mtvec\n"
    "  li t5, 32\n"
    "  csrs mie, t5\n"
    "  li t5, 128\n"
    "  csrs mstatus, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_switch_mtvec:\n"
    "  li t5, 2\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_clear_stip_on_bkp\n"
    "  la t5, ai_ibc_mtvec_new_vectored_table\n"
    "  ori t5, t5, 1\n"
    "  csrw mtvec, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_clear_stip_on_bkp:\n"
    "  li t5, 4\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_clear_slot0_on_bkp\n"
    "  li t5, 32\n"
    "  csrc mip, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_clear_slot0_on_bkp:\n"
    "  li t5, 8\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_skip_lsq_young\n"
    "  li t5, 0\n"
    "  csrw tselect, t5\n"
    "  csrw tdata1, x0\n"
    "  csrw tdata2, x0\n"
    "  la t5, ai_ibc_breakpoint_cleared_trigger_hits\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_skip_lsq_young:\n"
    "  li t5, 9\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_status_mix\n"
    "  la t5, ai_ibc_lsq_store_pair_after_young_site\n"
    "  csrw mepc, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_status_mix:\n"
    "  li t5, 10\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_log_vstart_bkp\n"
    "  li t5, 128\n"
    "  csrs mstatus, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_log_vstart_bkp:\n"
    "  li t5, 11\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_vector_dirty_bkp\n"
    "  csrr t5, vstart\n"
    "  la t6, ai_ibc_trap_vstart\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, 9f\n"
    "  addi t4, t4, -1\n"
    "  slli t4, t4, 3\n"
    "  add t6, t6, t4\n"
    "  sd t5, 0(t6)\n"
    "  csrwi vstart, 0\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_vector_dirty_bkp:\n"
    "  li t5, 13\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_hs_bkp_open_mei_stack\n"
    "  la t5, ai_ibc_vector_load64_dirty_after_site\n"
    "  csrw mepc, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_hs_bkp_open_mei_stack:\n"
    "  li t5, 12\n"
    "  bne t4, t5, .Lai_ibc_log_handler_check_sfence_clear_slot0\n"
    "  la t5, ai_ibc_priv_stack_outer_mepc\n"
    "  sd t1, 0(t5)\n"
    "  csrr t5, mstatus\n"
    "  la t6, ai_ibc_priv_stack_outer_mstatus\n"
    "  sd t5, 0(t6)\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000800\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 2048\n"
    ".Lai_ibc_log_handler_hs_wait_meip:\n"
    "  csrr t6, mip\n"
    "  li t3, 2048\n"
    "  and t6, t6, t3\n"
    "  bnez t6, .Lai_ibc_log_handler_hs_meip_seen\n"
    "  addi t5, t5, -1\n"
    "  bnez t5, .Lai_ibc_log_handler_hs_wait_meip\n"
    ".Lai_ibc_log_handler_hs_meip_seen:\n"
    "  la t5, ai_ibc_priv_stack_bp_returns\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  li t5, 2048\n"
    "  csrs mie, t5\n"
    "  li t5, 8\n"
    "  csrs mstatus, t5\n"
    "  li t5, 2048\n"
    ".Lai_ibc_log_handler_hs_wait_mei_return:\n"
    "  la t3, ai_ibc_priv_stack_mei_returns\n"
    "  ld t6, 0(t3)\n"
    "  bnez t6, .Lai_ibc_log_handler_hs_restore_outer\n"
    "  addi t5, t5, -1\n"
    "  bnez t5, .Lai_ibc_log_handler_hs_wait_mei_return\n"
    ".Lai_ibc_log_handler_hs_restore_outer:\n"
    "  la t5, ai_ibc_priv_stack_outer_mepc\n"
    "  ld t6, 0(t5)\n"
    "  csrw mepc, t6\n"
    "  la t5, ai_ibc_priv_stack_outer_mstatus\n"
    "  ld t6, 0(t5)\n"
    "  csrw mstatus, t6\n"
    "  csrr t6, mstatus\n"
    "  la t5, ai_ibc_priv_stack_final_mstatus\n"
    "  sd t6, 0(t5)\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_sfence_clear_slot0:\n"
    "  li t5, 16\n"
    "  bne t4, t5, 9f\n"
    "  la t5, ai_ibc_pmp_repair_addr0\n"
    "  ld t6, 0(t5)\n"
    "  csrw pmpaddr0, t6\n"
    "  la t5, ai_ibc_pmp_repair_addr1\n"
    "  ld t6, 0(t5)\n"
    "  csrw pmpaddr1, t6\n"
    "  la t5, ai_ibc_pmp_repair_addr2\n"
    "  ld t6, 0(t5)\n"
    "  csrw pmpaddr2, t6\n"
    "  la t5, ai_ibc_pmp_repair_cfg\n"
    "  ld t6, 0(t5)\n"
    "  csrw pmpcfg0, t6\n"
    "  la t5, ai_ibc_pmp_repair_hits\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  j 9f\n"
    "3:\n"
    "  li t3, 2\n"
    "  bne t0, t3, .Lai_ibc_log_handler_check_interrupt\n"
    "  la t3, ai_ibc_handler_mode\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 6\n"
    "  bne t4, t5, 9f\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_log_handler_skip_illegal_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_log_handler_skip_illegal_done\n"
    ".Lai_ibc_log_handler_skip_illegal_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_log_handler_skip_illegal_done:\n"
    "  csrw mepc, t1\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_interrupt:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, 9f\n"
    "  lw t4, 0(t1)\n"
    "  li t5, 0x10500073\n"
    "  bne t4, t5, 6f\n"
    "  addi t1, t1, 4\n"
    "  csrw mepc, t1\n"
    "6:\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 1\n"
    "  bne t3, t4, .Lai_ibc_log_handler_check_msi\n"
    "  li t5, 2\n"
    "  csrc mip, t5\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_check_msi:\n"
    "  li t4, 3\n"
    "  bne t3, t4, 7f\n"
    "  li t5, 8\n"
    "  csrc mip, t5\n"
    "  j 9f\n"
    "7:\n"
    "  li t4, 5\n"
    "  bne t3, t4, 8f\n"
    "  la t4, ai_ibc_handler_mode\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 3\n"
    "  bne t5, t6, .Lai_ibc_log_handler_clear_stip\n"
    "  li t5, 0\n"
    "  csrw tselect, t5\n"
    "  csrw tdata1, x0\n"
    "  csrw tdata2, x0\n"
    "  la t5, ai_ibc_interrupt_cleared_trigger_hits\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    ".Lai_ibc_log_handler_clear_stip:\n"
    "  li t5, 32\n"
    "  csrc mip, t5\n"
    "  j 9f\n"
    "8:\n"
    "  li t4, 11\n"
    "  bne t3, t4, 9f\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
  "  la t4, ai_ibc_handler_mode\n"
  "  ld t5, 0(t4)\n"
  "  li t6, 10\n"
  "  beq t5, t6, .Lai_ibc_log_handler_mei_status_mix\n"
  "  li t6, 12\n"
  "  beq t5, t6, .Lai_ibc_log_handler_mei_priv_stack\n"
  "  li t6, 13\n"
  "  bne t5, t6, 9f\n"
  "  csrr t5, mstatus\n"
  "  la t6, ai_ibc_vector_dirty_mei_mstatus\n"
  "  sd t5, 0(t6)\n"
  "  j 9f\n"
  ".Lai_ibc_log_handler_mei_priv_stack:\n"
  "  csrr t5, mstatus\n"
  "  la t6, ai_ibc_priv_stack_after_mei_mstatus\n"
    "  sd t5, 0(t6)\n"
    "  la t5, ai_ibc_priv_stack_mei_returns\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  j 9f\n"
    ".Lai_ibc_log_handler_mei_status_mix:\n"
    "  li t5, 128\n"
    "  csrc mstatus, t5\n"
    "9:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    ".Lai_ibc_log_handler_framework_ecall:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  j mhandler_entry\n"
    ".align 2\n"
    ".global ai_ibc_stvec_log_ssip_handler\n"
    "ai_ibc_stvec_log_ssip_handler:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, scause\n"
    "  csrr t1, sepc\n"
    "  csrr t2, stval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_stvec_ssip_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, sip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x4942434e45534800\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, sstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_stvec_ssip_log_done:\n"
    "  li t3, 3\n"
  "  bne t0, t3, .Lai_ibc_stvec_ssip_check_interrupt\n"
  "  lhu t4, 0(t1)\n"
  "  andi t4, t4, 3\n"
  "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_stvec_ssip_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_stvec_ssip_bkp_done\n"
    ".Lai_ibc_stvec_ssip_bkp_16:\n"
    "  addi t1, t1, 2\n"
  ".Lai_ibc_stvec_ssip_bkp_done:\n"
  "  csrw sepc, t1\n"
  "  la t3, ai_ibc_handler_mode\n"
  "  ld t4, 0(t3)\n"
  "  li t5, 15\n"
  "  bne t4, t5, .Lai_ibc_stvec_ssip_exit\n"
  "  la t5, ai_ibc_sfence_target_pte\n"
  "  ld t6, 0(t5)\n"
  "  beqz t6, .Lai_ibc_stvec_ssip_sfence_only\n"
  "  la t5, ai_ibc_sfence_new_pte\n"
  "  ld t5, 0(t5)\n"
  "  sd t5, 0(t6)\n"
  ".Lai_ibc_stvec_ssip_sfence_only:\n"
  "  sfence.vma\n"
  "  la t5, ai_ibc_sfence_handler_hits\n"
  "  ld t6, 0(t5)\n"
  "  addi t6, t6, 1\n"
  "  sd t6, 0(t5)\n"
  "  j .Lai_ibc_stvec_ssip_exit\n"
    ".Lai_ibc_stvec_ssip_check_interrupt:\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 1\n"
    "  bne t3, t4, .Lai_ibc_stvec_ssip_exit\n"
    "  li t5, 2\n"
    "  csrc sip, t5\n"
    ".Lai_ibc_stvec_ssip_exit:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  sret\n"
    ".align 2\n"
    ".global ai_ibc_stip_handler_ebreak_probe\n"
    "ai_ibc_stip_handler_ebreak_probe:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_nested_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x4942434e45535400\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_nested_log_done:\n"
    "  li t3, 3\n"
    "  beq t0, t3, .Lai_ibc_nested_breakpoint\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_nested_exit\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 5\n"
    "  bne t3, t4, .Lai_ibc_nested_exit\n"
    "  la t5, ai_ibc_nested_outer_epc\n"
    "  sd t1, 0(t5)\n"
    "  csrr t5, mstatus\n"
    "  la t6, ai_ibc_nested_outer_mstatus\n"
    "  sd t5, 0(t6)\n"
    ".global ai_ibc_nested_handler_ebreak_site\n"
    "ai_ibc_nested_handler_ebreak_site:\n"
    "  ebreak\n"
    ".global ai_ibc_nested_handler_after_ebreak\n"
    "ai_ibc_nested_handler_after_ebreak:\n"
    "  li t5, 0x73746962\n"
    "  la t6, ai_ibc_nested_after_marker\n"
    "  sd t5, 0(t6)\n"
    "  li t5, 32\n"
    "  csrc mip, t5\n"
    "  la t5, ai_ibc_nested_outer_epc\n"
    "  ld t1, 0(t5)\n"
    "  csrw mepc, t1\n"
    "  la t5, ai_ibc_nested_outer_mstatus\n"
    "  ld t1, 0(t5)\n"
    "  csrw mstatus, t1\n"
    "  j .Lai_ibc_nested_exit\n"
    ".Lai_ibc_nested_breakpoint:\n"
    "  addi t1, t1, 4\n"
    "  csrw mepc, t1\n"
    ".Lai_ibc_nested_exit:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
    ".align 6\n"
    ".option push\n"
    ".option norvc\n"
    ".global ai_ibc_mtvec_vectored_table\n"
    "ai_ibc_mtvec_vectored_table:\n"
    "  j ai_ibc_mtvec_vectored_direct_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_stip_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    ".align 2\n"
    "ai_ibc_mtvec_vectored_direct_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_vectored_direct_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_vectored_direct_log_done:\n"
    "  la t3, ai_ibc_vectored_direct_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_vectored_direct_unexpected\n"
    "  addi t1, t1, 4\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_vectored_exit\n"
    ".Lai_ibc_vectored_direct_unexpected:\n"
    "  la t3, ai_ibc_vectored_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  j .Lai_ibc_vectored_exit\n"
    "ai_ibc_mtvec_vectored_stip_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_vectored_stip_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_vectored_stip_log_done:\n"
    "  la t3, ai_ibc_vectored_stip_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 5\n"
    "  bne t3, t4, .Lai_ibc_vectored_stip_unexpected\n"
    "  li t5, 32\n"
    "  csrc mip, t5\n"
    "  j .Lai_ibc_vectored_exit\n"
    ".Lai_ibc_vectored_stip_unexpected:\n"
    "  la t3, ai_ibc_vectored_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  j .Lai_ibc_vectored_exit\n"
    "ai_ibc_mtvec_vectored_unexpected_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_vectored_unexpected_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_vectored_unexpected_log_done:\n"
    "  la t3, ai_ibc_vectored_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_vectored_exit:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    ".align 6\n"
    ".global ai_ibc_mtvec_new_vectored_table\n"
    "ai_ibc_mtvec_new_vectored_table:\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_new_vectored_stip_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    "  j ai_ibc_mtvec_vectored_unexpected_entry\n"
    ".align 2\n"
    "ai_ibc_mtvec_new_vectored_stip_entry:\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_new_vector_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_new_vector_log_done:\n"
    "  la t3, ai_ibc_new_vectored_stip_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 5\n"
    "  bne t3, t4, .Lai_ibc_new_vector_unexpected\n"
    "  li t5, 32\n"
    "  csrc mip, t5\n"
    "  j .Lai_ibc_new_vector_exit\n"
    ".Lai_ibc_new_vector_unexpected:\n"
    "  la t3, ai_ibc_vectored_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_new_vector_exit:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_bkp_csr_owner_handler\n"
    "ai_ibc_rnmi_bkp_csr_owner_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_ibc_rnmi_owner_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_ibc_rnmi_owner_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_ibc_rnmi_owner_ordinary\n"
    "  csrr t2, 0x741\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_rnmi_owner_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_rnmi_ordinary_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x4942434e45535400\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    ".Lai_ibc_rnmi_ordinary_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_rnmi_check_mei\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_rnmi_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_rnmi_bkp_epc_done\n"
    ".Lai_ibc_rnmi_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_rnmi_bkp_epc_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_rnmi_ordinary_return\n"
    ".Lai_ibc_rnmi_check_mei:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_rnmi_ordinary_return\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 11\n"
    "  bne t3, t4, .Lai_ibc_rnmi_ordinary_return\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    ".Lai_ibc_rnmi_ordinary_return:\n"
    "  la t5, ai_ibc_rnmi_mret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_masked_rnmi_ordinary_snapshot_handler\n"
    "ai_ibc_masked_rnmi_ordinary_snapshot_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_masked_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, 0x742\n"
    "  la t6, ai_ibc_trap_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, 0x741\n"
    "  la t6, ai_ibc_trap_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, 0x744\n"
    "  la t6, ai_ibc_trap_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_masked_log_done:\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_masked_check_mei\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_masked_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_masked_bkp_done\n"
    ".Lai_ibc_masked_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_masked_bkp_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_masked_return\n"
    ".Lai_ibc_masked_check_mei:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_masked_return\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 11\n"
    "  bne t3, t4, .Lai_ibc_masked_return\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    ".Lai_ibc_masked_return:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_scratch_isolation_handler\n"
    "ai_ibc_rnmi_scratch_isolation_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_ibc_scratch_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_ibc_scratch_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_ibc_scratch_ordinary\n"
    "  csrr t2, 0x741\n"
    "  csrr t3, mscratch\n"
    "  csrr t4, 0x740\n"
    "  la t5, ai_ibc_scratch_rnmi_mscratch_seen\n"
    "  sd t3, 0(t5)\n"
    "  la t5, ai_ibc_scratch_rnmi_mnscratch_before\n"
    "  sd t4, 0(t5)\n"
    "  li t4, 0x6e736372726e0101\n"
    "  csrw 0x740, t4\n"
    "  la t5, ai_ibc_scratch_rnmi_mnscratch_written\n"
    "  sd t4, 0(t5)\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_scratch_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_scratch_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_scratch_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_scratch_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    ".Lai_ibc_scratch_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_scratch_check_mei\n"
    "  la t3, ai_ibc_scratch_bp_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  csrr t4, mscratch\n"
    "  la t5, ai_ibc_scratch_bp_mscratch_before\n"
    "  sd t4, 0(t5)\n"
    "  li t4, 0x6d73637262700001\n"
    "  csrw mscratch, t4\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000001\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_hits\n"
    "  li t6, 4096\n"
    ".Lai_ibc_scratch_wait_rnmi:\n"
    "  ld t4, 0(t5)\n"
    "  bnez t4, .Lai_ibc_scratch_wait_rnmi_done\n"
    "  addi t6, t6, -1\n"
    "  bnez t6, .Lai_ibc_scratch_wait_rnmi\n"
    ".Lai_ibc_scratch_wait_rnmi_done:\n"
    "  csrr t4, mscratch\n"
    "  la t5, ai_ibc_scratch_bp_mscratch_after_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  csrr t4, 0x740\n"
    "  la t5, ai_ibc_scratch_bp_mnscratch_after_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_scratch_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_scratch_bkp_done\n"
    ".Lai_ibc_scratch_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_scratch_bkp_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_scratch_return\n"
    ".Lai_ibc_scratch_check_mei:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_scratch_unexpected\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 11\n"
    "  bne t3, t4, .Lai_ibc_scratch_unexpected\n"
    "  la t3, ai_ibc_scratch_mei_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  csrr t4, mscratch\n"
    "  la t5, ai_ibc_scratch_mei_mscratch_before\n"
    "  sd t4, 0(t5)\n"
    "  li t4, 0x6d7363726d656901\n"
    "  csrw mscratch, t4\n"
    "  csrr t4, 0x740\n"
    "  la t5, ai_ibc_scratch_mei_mnscratch_seen\n"
    "  sd t4, 0(t5)\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  j .Lai_ibc_scratch_return\n"
    ".Lai_ibc_scratch_unexpected:\n"
    "  la t3, ai_ibc_scratch_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_scratch_return:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
);

asm(
    ".section .text\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_source_clear_order_handler\n"
    "ai_ibc_rnmi_source_clear_order_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_ibc_source_order_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_ibc_source_order_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_ibc_source_order_ordinary\n"
    "  csrr t2, 0x741\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_source_order_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_source_order_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_source_order_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_source_order_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x4942434e45535400\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    ".Lai_ibc_source_order_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_source_order_check_mei\n"
    "  la t3, ai_ibc_source_order_bp_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  csrr t4, mip\n"
    "  la t5, ai_ibc_source_order_bp_mip_before_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000001\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_hits\n"
    "  li t6, 4096\n"
    ".Lai_ibc_source_order_wait_rnmi:\n"
    "  ld t4, 0(t5)\n"
    "  bnez t4, .Lai_ibc_source_order_wait_rnmi_done\n"
    "  addi t6, t6, -1\n"
    "  bnez t6, .Lai_ibc_source_order_wait_rnmi\n"
    ".Lai_ibc_source_order_wait_rnmi_done:\n"
    "  csrr t4, mstatus\n"
    "  la t5, ai_ibc_source_order_bp_mstatus_after_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  csrr t4, mie\n"
    "  la t5, ai_ibc_source_order_bp_mie_after_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  csrr t4, mip\n"
    "  la t5, ai_ibc_source_order_bp_mip_after_rnmi\n"
    "  sd t4, 0(t5)\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_source_order_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_source_order_bkp_done\n"
    ".Lai_ibc_source_order_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_source_order_bkp_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_source_order_return\n"
    ".Lai_ibc_source_order_check_mei:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_source_order_unexpected\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 11\n"
    "  bne t3, t4, .Lai_ibc_source_order_unexpected\n"
    "  la t3, ai_ibc_source_order_mei_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  csrr t4, mip\n"
    "  la t5, ai_ibc_source_order_mei_mip_before_clear\n"
    "  sd t4, 0(t5)\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_handler_mode\n"
    "  ld t6, 0(t5)\n"
    "  li t5, 14\n"
    "  bne t6, t5, .Lai_ibc_source_order_skip_clear_rnmi\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000001\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_hits\n"
    "  li t6, 4096\n"
    ".Lai_ibc_source_order_wait_clear_rnmi:\n"
    "  ld t4, 0(t5)\n"
    "  bnez t4, .Lai_ibc_source_order_skip_clear_rnmi\n"
    "  addi t6, t6, -1\n"
    "  bnez t6, .Lai_ibc_source_order_wait_clear_rnmi\n"
    ".Lai_ibc_source_order_skip_clear_rnmi:\n"
    "  csrr t4, mip\n"
    "  la t5, ai_ibc_source_order_mei_mip_after_clear\n"
    "  sd t4, 0(t5)\n"
    "  j .Lai_ibc_source_order_return\n"
    ".Lai_ibc_source_order_unexpected:\n"
    "  la t3, ai_ibc_source_order_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_source_order_return:\n"
    "  la t5, ai_ibc_rnmi_mret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
);

asm(
    ".section .text\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_mtvec_switch_alt_mei_handler\n"
    "ai_ibc_rnmi_mtvec_switch_alt_mei_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_alt_mei_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x494243414c544d00\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_alt_mei_log_done:\n"
    "  srli t3, t0, 63\n"
    "  beqz t3, .Lai_ibc_alt_mei_unexpected\n"
    "  slli t3, t0, 1\n"
    "  srli t3, t3, 1\n"
    "  li t4, 11\n"
    "  bne t3, t4, .Lai_ibc_alt_mei_unexpected\n"
    "  la t3, ai_ibc_rnmi_switch_alt_mei_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  csrr t4, mtvec\n"
    "  la t5, ai_ibc_rnmi_switch_alt_mtvec_seen\n"
    "  sd t4, 0(t5)\n"
    "  li t5, 0xffff000100001000\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  j .Lai_ibc_alt_mei_return\n"
    ".Lai_ibc_alt_mei_rnmi_record:\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_ibc_alt_mei_unexpected\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_ibc_alt_mei_unexpected\n"
    "  csrr t2, 0x741\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_alt_mei_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_alt_mei_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t5, 0x524e4d4953564302\n"
    "  la t6, ai_ibc_rnmi_switch_rnmi_entry_id\n"
    "  sd t5, 0(t6)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_alt_mei_unexpected:\n"
    "  la t3, ai_ibc_rnmi_switch_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_alt_mei_return:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
    ".align 2\n"
    ".global ai_ibc_rnmi_mtvec_switch_handler\n"
    "ai_ibc_rnmi_mtvec_switch_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_ibc_switch_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_ibc_switch_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_ibc_switch_ordinary\n"
    "  csrr t2, 0x741\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_switch_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_switch_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  li t5, 0x524e4d4953564301\n"
    "  la t6, ai_ibc_rnmi_switch_rnmi_entry_id\n"
    "  sd t5, 0(t6)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000000\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_switch_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_switch_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_ibc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x49424353574d5400\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_switch_log_done:\n"
    "  li t3, 3\n"
    "  bne t0, t3, .Lai_ibc_switch_unexpected\n"
    "  la t3, ai_ibc_rnmi_switch_bp_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t4, ai_ibc_rnmi_mtvec_switch_alt_mei_handler\n"
    "  csrw mtvec, t4\n"
    "  csrr t4, mtvec\n"
    "  la t5, ai_ibc_rnmi_switch_mtvec_after_bkp\n"
    "  sd t4, 0(t5)\n"
    "  li t5, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t5, 0xffff000000000001\n"
    "  add x0, x0, t5\n"
    "  fence.i\n"
    "  li t6, 4096\n"
    ".Lai_ibc_switch_wait_rnmi:\n"
    "  la t4, ai_ibc_rnmi_hits\n"
    "  ld t5, 0(t4)\n"
    "  bnez t5, .Lai_ibc_switch_wait_done\n"
    "  addi t6, t6, -1\n"
    "  bnez t6, .Lai_ibc_switch_wait_rnmi\n"
    ".Lai_ibc_switch_wait_done:\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_switch_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_switch_bkp_done\n"
    ".Lai_ibc_switch_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_switch_bkp_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_switch_return\n"
    ".Lai_ibc_switch_unexpected:\n"
    "  la t3, ai_ibc_rnmi_switch_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_switch_return:\n"
    "  la t5, ai_ibc_rnmi_mret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
);

asm(
    ".section .text\n"
    ".align 2\n"
    ".global ai_ibc_unexpected_rnmi_bkp_fault_handler\n"
    "ai_ibc_unexpected_rnmi_bkp_fault_handler:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  addi sp, sp, -64\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  beqz t2, .Lai_ibc_unexp_rnmi_record\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_ibc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_unexp_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_ibc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mstatus\n"
    "  la t6, ai_ibc_trap_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x494243554e455800\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_ibc_trap_magic\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    ".Lai_ibc_unexp_log_done:\n"
    "  li t3, 3\n"
    "  beq t0, t3, .Lai_ibc_unexp_bkp\n"
    "  li t3, 2\n"
    "  bne t0, t3, .Lai_ibc_unexp_ordinary_unexpected\n"
    "  la t3, ai_ibc_unexpected_rnmi_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_unexp_ili_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_unexp_ili_done\n"
    ".Lai_ibc_unexp_ili_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_unexp_ili_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_unexp_return\n"
    ".Lai_ibc_unexp_bkp:\n"
    "  la t3, ai_ibc_unexpected_rnmi_bkp_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  .4byte 0xffffffff\n"
    "  la t3, ai_ibc_unexpected_rnmi_after_fault_marker\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  lhu t4, 0(t1)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_unexp_bkp_16\n"
    "  addi t1, t1, 4\n"
    "  j .Lai_ibc_unexp_bkp_done\n"
    ".Lai_ibc_unexp_bkp_16:\n"
    "  addi t1, t1, 2\n"
    ".Lai_ibc_unexp_bkp_done:\n"
    "  csrw mepc, t1\n"
    "  j .Lai_ibc_unexp_return\n"
    ".Lai_ibc_unexp_ordinary_unexpected:\n"
    "  la t3, ai_ibc_unexpected_rnmi_unexpected_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  j .Lai_ibc_unexp_return\n"
    ".Lai_ibc_unexp_rnmi_record:\n"
    "  csrr t1, 0x742\n"
    "  csrr t2, 0x741\n"
    "  la t3, ai_ibc_rnmi_hits\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 8\n"
    "  bgeu t4, t5, .Lai_ibc_unexp_rnmi_log_done\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_ibc_rnmi_mncause\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnepc\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_ibc_rnmi_mnstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_ibc_unexp_rnmi_log_done:\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t3, ai_ibc_unexpected_rnmi_fault_hits\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  lhu t4, 0(t2)\n"
    "  andi t4, t4, 3\n"
    "  li t5, 3\n"
    "  bne t4, t5, .Lai_ibc_unexp_rnmi_16\n"
    "  addi t2, t2, 4\n"
    "  j .Lai_ibc_unexp_rnmi_epc_done\n"
    ".Lai_ibc_unexp_rnmi_16:\n"
    "  addi t2, t2, 2\n"
    ".Lai_ibc_unexp_rnmi_epc_done:\n"
    "  csrw 0x741, t2\n"
    "  la t5, ai_ibc_rnmi_mnret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_ibc_unexp_return:\n"
    "  la t5, ai_ibc_rnmi_mret_markers\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "  .option pop\n"
);

static inline void ai_ibc_reset_log(void)
{
    ai_ibc_trap_count = 0ULL;
    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        ai_ibc_trap_cause[i] = 0ULL;
        ai_ibc_trap_epc[i] = 0ULL;
        ai_ibc_trap_tval[i] = 0ULL;
        ai_ibc_trap_vstart[i] = 0ULL;
        ai_ibc_trap_mip[i] = 0ULL;
        ai_ibc_trap_magic[i] = 0ULL;
        ai_ibc_trap_mstatus[i] = 0ULL;
        ai_ibc_trap_mncause[i] = 0ULL;
        ai_ibc_trap_mnepc[i] = 0ULL;
        ai_ibc_trap_mnstatus[i] = 0ULL;
        ai_ibc_trap_pmpcfg0[i] = 0ULL;
        ai_ibc_trap_pmpaddr0[i] = 0ULL;
        ai_ibc_trap_pmpaddr1[i] = 0ULL;
        ai_ibc_rnmi_mncause[i] = 0ULL;
        ai_ibc_rnmi_mnepc[i] = 0ULL;
        ai_ibc_rnmi_mnstatus[i] = 0ULL;
        ai_ibc_counter_cycle[i] = 0ULL;
        ai_ibc_counter_instret[i] = 0ULL;
        ai_ibc_counter_inhibit_before[i] = 0ULL;
        ai_ibc_counter_inhibit_after[i] = 0ULL;
        ai_ibc_counter_inhibit_restored[i] = 0ULL;
    }
    ai_ibc_rnmi_hits = 0ULL;
    ai_ibc_rnmi_mnret_markers = 0ULL;
    ai_ibc_rnmi_mret_markers = 0ULL;
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_nested_after_marker = 0ULL;
    ai_ibc_nested_outer_epc = 0ULL;
    ai_ibc_nested_outer_mstatus = 0ULL;
    ai_ibc_vectored_direct_hits = 0ULL;
    ai_ibc_vectored_stip_hits = 0ULL;
    ai_ibc_vectored_unexpected_hits = 0ULL;
    ai_ibc_new_vectored_stip_hits = 0ULL;
    ai_ibc_interrupt_cleared_trigger_hits = 0ULL;
    ai_ibc_breakpoint_cleared_trigger_hits = 0ULL;
    ai_ibc_scratch_bp_hits = 0ULL;
    ai_ibc_scratch_mei_hits = 0ULL;
    ai_ibc_scratch_unexpected_hits = 0ULL;
    ai_ibc_scratch_bp_mscratch_before = 0ULL;
    ai_ibc_scratch_bp_mscratch_after_rnmi = 0ULL;
    ai_ibc_scratch_bp_mnscratch_after_rnmi = 0ULL;
    ai_ibc_scratch_rnmi_mscratch_seen = 0ULL;
    ai_ibc_scratch_rnmi_mnscratch_before = 0ULL;
    ai_ibc_scratch_rnmi_mnscratch_written = 0ULL;
    ai_ibc_scratch_mei_mscratch_before = 0ULL;
    ai_ibc_scratch_mei_mnscratch_seen = 0ULL;
    ai_ibc_source_order_bp_hits = 0ULL;
    ai_ibc_source_order_mei_hits = 0ULL;
    ai_ibc_source_order_unexpected_hits = 0ULL;
    ai_ibc_source_order_bp_mip_before_rnmi = 0ULL;
    ai_ibc_source_order_bp_mip_after_rnmi = 0ULL;
    ai_ibc_source_order_bp_mstatus_after_rnmi = 0ULL;
    ai_ibc_source_order_bp_mie_after_rnmi = 0ULL;
    ai_ibc_source_order_mei_mip_before_clear = 0ULL;
    ai_ibc_source_order_mei_mip_after_clear = 0ULL;
    ai_ibc_rnmi_switch_bp_hits = 0ULL;
    ai_ibc_rnmi_switch_alt_mei_hits = 0ULL;
    ai_ibc_rnmi_switch_unexpected_hits = 0ULL;
    ai_ibc_rnmi_switch_rnmi_entry_id = 0ULL;
    ai_ibc_rnmi_switch_mtvec_after_bkp = 0ULL;
    ai_ibc_rnmi_switch_alt_mtvec_seen = 0ULL;
    ai_ibc_unexpected_rnmi_bkp_hits = 0ULL;
    ai_ibc_unexpected_rnmi_fault_hits = 0ULL;
    ai_ibc_unexpected_rnmi_after_fault_marker = 0ULL;
    ai_ibc_unexpected_rnmi_unexpected_hits = 0ULL;
    ai_ibc_priv_stack_outer_mepc = 0ULL;
    ai_ibc_priv_stack_outer_mstatus = 0ULL;
    ai_ibc_priv_stack_after_mei_mstatus = 0ULL;
    ai_ibc_priv_stack_final_mstatus = 0ULL;
    ai_ibc_priv_stack_bp_returns = 0ULL;
    ai_ibc_priv_stack_mei_returns = 0ULL;
    ai_ibc_vector_dirty_mstatus_before = 0ULL;
    ai_ibc_vector_dirty_mstatus_after = 0ULL;
    ai_ibc_vector_dirty_mei_mstatus = 0ULL;
    ai_ibc_pmp_repair_addr0 = 0ULL;
    ai_ibc_pmp_repair_addr1 = 0ULL;
    ai_ibc_pmp_repair_addr2 = 0ULL;
    ai_ibc_pmp_repair_cfg = 0ULL;
    ai_ibc_pmp_repair_hits = 0ULL;
    ai_ibc_sfence_target_pte = 0ULL;
    ai_ibc_sfence_new_pte = 0ULL;
    ai_ibc_sfence_handler_hits = 0ULL;
}

static inline bool ai_ibc_log_has_cause(uint64_t cause)
{
    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause) {
            return true;
        }
    }

    return false;
}

static inline bool ai_ibc_log_has_cause_epc(uint64_t cause, uintptr_t epc)
{
    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_epc[i] == epc) {
            return true;
        }
    }

    return false;
}

static inline uint64_t ai_ibc_log_count_cause_epc(uint64_t cause,
    uintptr_t epc)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_epc[i] == epc) {
            ++count;
        }
    }

    return count;
}

static inline uint64_t ai_ibc_log_count_cause_tval(uint64_t cause,
    uint64_t tval)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_tval[i] == tval) {
            ++count;
        }
    }

    return count;
}

static inline bool ai_ibc_log_has_cause_tval_pmp_snapshot(uint64_t cause,
    uint64_t tval,
    uint64_t pmpcfg0,
    uint64_t pmpaddr0,
    uint64_t pmpaddr1)
{
    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_tval[i] == tval &&
            ai_ibc_trap_pmpcfg0[i] == pmpcfg0 &&
            ai_ibc_trap_pmpaddr0[i] == pmpaddr0 &&
            ai_ibc_trap_pmpaddr1[i] == pmpaddr1) {
            return true;
        }
    }

    return false;
}

static inline bool ai_ibc_log_has_cause_pmp_snapshot(uint64_t cause,
    uint64_t pmpcfg0,
    uint64_t pmpaddr0,
    uint64_t pmpaddr1)
{
    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_pmpcfg0[i] == pmpcfg0 &&
            ai_ibc_trap_pmpaddr0[i] == pmpaddr0 &&
            ai_ibc_trap_pmpaddr1[i] == pmpaddr1) {
            return true;
        }
    }

    return false;
}

static inline uint64_t ai_ibc_log_count_cause_epc_tval(uint64_t cause,
    uintptr_t epc,
    uint64_t tval)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause &&
            ai_ibc_trap_epc[i] == epc &&
            ai_ibc_trap_tval[i] == tval) {
            ++count;
        }
    }

    return count;
}

static inline uint64_t ai_ibc_log_count_bkp_tval_vstart(uint64_t tval,
    uint64_t vstart)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == CAUSE_BKP &&
            ai_ibc_trap_tval[i] == tval &&
            ai_ibc_trap_vstart[i] == vstart) {
            ++count;
        }
    }

    return count;
}

static inline uint64_t ai_ibc_log_count_cause(uint64_t cause)
{
    uint64_t count = 0ULL;

    for (unsigned i = 0; i < AI_IBC_LOG_SLOTS; ++i) {
        if (ai_ibc_trap_cause[i] == cause) {
            ++count;
        }
    }

    return count;
}

static inline void ai_ibc_vmodule_issue_addr_cmd(uint64_t offset)
{
    const uint64_t addr_cmd =
        AI_IBC_VMODULE_ADDR_CMD_PREFIX | (offset & 0xffffffffULL);

    asm volatile(
        "mv t0, %[addr]\n\t"
        "fence.i\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        :
        : [addr] "r"(addr_cmd)
        : "t0", "memory");
}

static inline void ai_ibc_vmodule_issue_data_cmd(uint64_t data)
{
    const uint64_t data_cmd =
        AI_IBC_VMODULE_DATA_CMD_PREFIX | (data & 0xffffffffULL);

    asm volatile(
        "mv t0, %[data]\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        :
        : [data] "r"(data_cmd)
        : "t0", "memory");
}

static inline void ai_ibc_vmodule_write_reg(uint64_t offset, uint64_t data)
{
    ai_ibc_vmodule_issue_addr_cmd(offset);
    ai_ibc_vmodule_issue_data_cmd(data);
}

static inline void ai_ibc_vmodule_clear_main_interrupt(void)
{
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
}

static inline void ai_ibc_vmodule_set_delay(uint64_t delay_cycles)
{
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_INTERRUPT_DELAY,
        delay_cycles);
}

static inline void ai_ibc_prepare_vmodule_meip_pending(bool local_enable,
    uint64_t delay_cycles)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRC(CSR_MIE, MIE_MEIE);
    ai_ibc_vmodule_clear_main_interrupt();
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_vmodule_set_delay(delay_cycles);
    if (local_enable) {
        CSRS(CSR_MIE, MIE_MEIE);
    }
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
}

static bool ai_ibc_rnmi_breakpoint_length_round(bool compressed,
    uint64_t expected_marker,
    uintptr_t breakpoint_site,
    volatile uint64_t *marker)
{
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    *marker = 0ULL;

    if (compressed) {
        ai_ibc_rnmi_cebreak_marker_probe(marker);
    } else {
        ai_ibc_rnmi_ebreak_marker_probe(marker);
    }

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rnmi_owner_and_return =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        ai_ibc_rnmi_mnepc[0] != breakpoint_site &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL;
    const bool breakpoint_owner_and_return =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, breakpoint_site) == 1ULL &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_trap_epc[0] == breakpoint_site &&
        ai_ibc_trap_epc[0] != ai_ibc_rnmi_mnepc[0] &&
        ai_ibc_rnmi_mret_markers == 1ULL;
    const bool marker_after_actual_length =
        *marker == expected_marker;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool cleared_source_no_reentry =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);

    return rnmi_owner_and_return &&
        breakpoint_owner_and_return &&
        marker_after_actual_length &&
        cleared_source_no_reentry;
}

static bool ai_ibc_rnmi_trigger_half_config_round(bool write_tdata2_first,
    uint64_t tag)
{
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x726e6d6974636700ULL | tag;
    const uint64_t target_seed = 0x726e6d6974637400ULL | tag;
    const uint64_t after_seed = 0x726e6d6974636100ULL | tag;
    const uint64_t half_payload = 0x726e6d6974636800ULL | tag;
    const uint64_t blocked_payload = 0x726e6d6974636200ULL | tag;
    const uint64_t committed_payload = 0x726e6d6974636300ULL | tag;
    const uint64_t armed_tdata1 =
        AI_IBC_MCONTROL6_MSU_BASE | AI_IBC_MCONTROL6_STORE;
    uint64_t half_tdata1 = 0ULL;
    uint64_t half_tdata2 = 0ULL;
    uint64_t final_tdata1 = 0ULL;
    uint64_t final_tdata2 = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    CSRW(CSR_TSELECT, 0ULL);
    if (write_tdata2_first) {
        asm volatile("csrw tdata1, x0");
        CSRW(CSR_TDATA2, target_addr);
    }
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_rnmi_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    ai_ibc_read_trigger_csr_slot(0, &half_tdata1, &half_tdata2);
    ai_ibc_store_once(target_addr, half_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rnmi_record_once =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnret_markers == 1ULL;
    const bool half_config_not_armed =
        (half_tdata1 & AI_IBC_MCONTROL6_STORE) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == half_payload &&
        read64(after_addr) == after_seed;

    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, target_addr);
    CSRW(CSR_TDATA1, armed_tdata1);
    ai_ibc_read_trigger_csr_slot(0, &final_tdata1, &final_tdata2);
    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool final_config_breaks_once =
        (final_tdata1 & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        final_tdata2 == target_addr &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_addr) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == half_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_store_once(target_addr, committed_payload);
    const bool cleared_trigger_commits =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    return rnmi_record_once &&
        half_config_not_armed &&
        final_config_breaks_once &&
        cleared_trigger_commits;
}

static bool ai_ibc_vmodule_meip_trigger_half_config_round(
    bool write_tdata2_first,
    uint64_t tag)
{
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x6d65697463676700ULL | tag;
    const uint64_t target_seed = 0x6d65697463677400ULL | tag;
    const uint64_t after_seed = 0x6d65697463676100ULL | tag;
    const uint64_t half_payload = 0x6d65697463676800ULL | tag;
    const uint64_t blocked_payload = 0x6d65697463676200ULL | tag;
    const uint64_t committed_payload = 0x6d65697463676300ULL | tag;
    const uint64_t armed_tdata1 =
        AI_IBC_MCONTROL6_MSU_BASE | AI_IBC_MCONTROL6_STORE;
    uint64_t half_tdata1 = 0ULL;
    uint64_t half_tdata2 = 0ULL;
    uint64_t half_selected_slot = 0ULL;
    uint64_t final_tdata1 = 0ULL;
    uint64_t final_tdata2 = 0ULL;
    uint64_t final_selected_slot = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    CSRW(CSR_TSELECT, 0ULL);
    asm volatile("csrw tdata1, x0");
    if (write_tdata2_first) {
        CSRW(CSR_TDATA2, target_addr);
    }

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    half_selected_slot =
        ai_ibc_read_trigger_csr_slot(0, &half_tdata1, &half_tdata2);
    ai_ibc_store_once(target_addr, half_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mei_interrupt_once_and_cleared =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool half_csr_snapshot_expected =
        half_selected_slot == 0ULL &&
        (half_tdata1 & AI_IBC_MCONTROL6_STORE) == 0ULL &&
        ((write_tdata2_first && half_tdata2 == target_addr) ||
            (!write_tdata2_first && half_tdata2 == 0ULL));
    const bool half_config_not_armed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == half_payload &&
        read64(after_addr) == after_seed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL;

    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, target_addr);
    CSRW(CSR_TDATA1, armed_tdata1);
    final_selected_slot =
        ai_ibc_read_trigger_csr_slot(0, &final_tdata1, &final_tdata2);
    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool final_config_breaks_once =
        final_selected_slot == 0ULL &&
        (final_tdata1 & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        final_tdata2 == target_addr &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_addr) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == half_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_store_once(target_addr, committed_payload);
    const bool cleared_trigger_commits =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    return mei_interrupt_once_and_cleared &&
        half_csr_snapshot_expected &&
        half_config_not_armed &&
        final_config_breaks_once &&
        cleared_trigger_commits;
}

static inline bool ai_ibc_wait_for_mip_bits(uint64_t bits, uint64_t max_iter)
{
    for (uint64_t i = 0; i < max_iter; ++i) {
        if ((CSRR(CSR_MIP) & bits) == bits) {
            return true;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return (CSRR(CSR_MIP) & bits) == bits;
}

static inline uint64_t ai_ibc_write_stip_until_meip_pending(
    uint64_t max_rounds,
    uint64_t *observed_mip)
{
    uint64_t rounds = 0ULL;
    uint64_t mip_snapshot = 0ULL;

    for (; rounds < max_rounds; ++rounds) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        asm volatile("addi x0, x0, 0" ::: "memory");
        mip_snapshot = CSRR(CSR_MIP);
        if (mip_snapshot & MIP_MEIP) {
            ++rounds;
            break;
        }
    }

    if (observed_mip != NULL) {
        *observed_mip = mip_snapshot;
    }

    return rounds;
}

static inline void ai_ibc_clear_mcontrol6_slot(size_t slot)
{
    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, 0ULL);
}

static inline void ai_ibc_clear_all_mcontrol6_slots(void)
{
    for (size_t slot = 0; slot < AI_IBC_TRIGGER_SLOTS; ++slot) {
        ai_ibc_clear_mcontrol6_slot(slot);
    }
}

static inline void ai_ibc_arm_mcontrol6_slot(size_t slot,
    uintptr_t addr,
    uint64_t access_bits,
    bool chain)
{
    uint64_t tdata1 = AI_IBC_MCONTROL6_MSU_BASE | access_bits;

    if (chain) {
        tdata1 |= AI_IBC_MCONTROL6_CHAIN;
    }

    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, tdata1);
}

static inline void ai_ibc_arm_mcontrol6_slot_priv(size_t slot,
    uintptr_t addr,
    uint64_t access_bits,
    uint64_t priv_bits,
    bool chain)
{
    uint64_t tdata1 = AI_IBC_MCONTROL6_MSU_BASE | access_bits;

    tdata1 &= ~AI_IBC_MCONTROL6_MSU;
    tdata1 |= priv_bits & AI_IBC_MCONTROL6_MSU;
    if (chain) {
        tdata1 |= AI_IBC_MCONTROL6_CHAIN;
    }

    goto_priv(PRIV_M);
    CSRW(CSR_TSELECT, slot);
    asm volatile("csrw tdata1, x0");
    CSRW(CSR_TDATA2, addr);
    CSRW(CSR_TDATA1, tdata1);
}

static inline void ai_ibc_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_IBC_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_ibc_read_trigger_csr_slot(size_t slot,
    uint64_t *tdata1,
    uint64_t *tdata2)
{
    CSRW(CSR_TSELECT, slot);
    if (tdata1 != NULL) {
        *tdata1 = CSRR(CSR_TDATA1);
    }
    if (tdata2 != NULL) {
        *tdata2 = CSRR(CSR_TDATA2);
    }

    return CSRR(CSR_TSELECT);
}

static inline uint64_t ai_ibc_pmp_window_cfg(uint64_t middle_cfg)
{
    return AI_IBC_PMP_TOR_RWX | (middle_cfg << 8U) |
        (AI_IBC_PMP_TOR_RWX << 16U);
}

static inline void ai_ibc_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_IBC_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_ibc_configure_pmp_window(uintptr_t start,
    uintptr_t end,
    uint64_t middle_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2U);
    CSRW(CSR_PMPADDR1, end >> 2U);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_ibc_pmp_window_cfg(middle_cfg));
}

static inline void ai_ibc_prepare_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
}

static inline void ai_ibc_enable_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IBC_MSTATUS_MPP_MASK);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_ibc_clear_mprv_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IBC_MSTATUS_MPP_MASK);
}

static inline pte_t *ai_ibc_hs_leaf_entry(enum test_page page)
{
#ifdef sv48
    return &hspt[4][page];
#else
    return &hspt[2][page];
#endif
}

static inline void ai_ibc_zero_pt(pte_t *pt)
{
    for (size_t i = 0; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline uint64_t ai_ibc_satp_mode(void)
{
#ifdef sv48
    return 0x9ULL << 60;
#else
    return 0x8ULL << 60;
#endif
}

static inline uint64_t ai_ibc_satp_for_root(pte_t *root)
{
    return ai_ibc_satp_mode() | (((uintptr_t)root) >> PAGE_SHIFT);
}

static inline void ai_ibc_install_satp_root(pte_t *root,
    pte_t *l1,
    pte_t *l2,
    pte_t *l3,
    uintptr_t vaddr,
    uintptr_t paddr,
    uint64_t leaf_flags)
{
    ai_ibc_zero_pt(root);
    ai_ibc_zero_pt(l1);
    ai_ibc_zero_pt(l2);
    ai_ibc_zero_pt(l3);

#ifdef sv48
    uintptr_t addr = 0ULL;

    root[PTE_INDEX(0, vaddr)] = PTE_V | (((uintptr_t)l1) >> 2);
    for (size_t i = 0; i < 4U; ++i) {
        l1[i] = (addr >> 2) | leaf_flags;
        addr += SUPERPAGE_SIZE(1);
    }
    l1[PTE_INDEX(1, vaddr)] = PTE_V | (((uintptr_t)l2) >> 2);
    l2[PTE_INDEX(2, vaddr)] = PTE_V | (((uintptr_t)l3) >> 2);
    l3[PTE_INDEX(3, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2) | leaf_flags;
#else
    uintptr_t addr = 0ULL;

    (void)l3;
    for (size_t i = 0; i < 4U; ++i) {
        root[i] = (addr >> 2) | leaf_flags;
        addr += SUPERPAGE_SIZE(0);
    }
    root[PTE_INDEX(0, vaddr)] = PTE_V | (((uintptr_t)l1) >> 2);
    l1[PTE_INDEX(1, vaddr)] = PTE_V | (((uintptr_t)l2) >> 2);
    l2[PTE_INDEX(2, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2) | leaf_flags;
#endif
}

static inline uintptr_t ai_ibc_pbmt_alias_anchor(enum test_page page,
    uintptr_t target_page_start)
{
    return target_page_start - ((uintptr_t)page * PAGE_SIZE);
}

static inline uintptr_t ai_ibc_pbmt_alias_vaddr(enum test_page page,
    uintptr_t paddr)
{
    return hs_page_base(page) + (paddr & (PAGE_SIZE - 1ULL));
}

static inline uintptr_t ai_ibc_prepare_pbmt_nc_alias_to_paddr(
    enum test_page page,
    uintptr_t paddr)
{
    const uintptr_t page_start = paddr & PAGE_ADDR_MSK;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    pbmt_hspt_to_x(page);
    hspt_leaf_change_base_paddr(page,
        ai_ibc_pbmt_alias_anchor(page, page_start));
    sfence_vma();

    return ai_ibc_pbmt_alias_vaddr(page, paddr);
}

static inline uint64_t ai_ibc_mcontrol6_type(uint64_t tdata1)
{
    return (tdata1 >> AI_IBC_MCONTROL6_TYPE_SHIFT) & 0xfULL;
}

static inline uint64_t ai_ibc_mcontrol6_action(uint64_t tdata1)
{
    return (tdata1 >> AI_IBC_MCONTROL6_ACTION_SHIFT) & 0xfULL;
}

static inline void ai_ibc_fence_i(void)
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

static inline void ai_ibc_write_smc_code(uint32_t value_inst)
{
    const uintptr_t code_addr = (uintptr_t)&ai_ibc_smc_code[0];

    write32(code_addr, 0x00000013U);
    write32(code_addr + sizeof(uint32_t), value_inst);
    write32(code_addr + 2U * sizeof(uint32_t), 0x00008067U);
    write32(code_addr + 3U * sizeof(uint32_t), 0x00000013U);
    asm volatile("fence rw, rw" ::: "memory");
}

static inline bool ai_ibc_smc_code_matches(uint32_t value_inst)
{
    const uintptr_t code_addr = (uintptr_t)&ai_ibc_smc_code[0];

    return read32(code_addr) == 0x00000013U &&
        read32(code_addr + sizeof(uint32_t)) == value_inst &&
        read32(code_addr + 2U * sizeof(uint32_t)) == 0x00008067U &&
        read32(code_addr + 3U * sizeof(uint32_t)) == 0x00000013U;
}

static inline void ai_ibc_prepare_stip_pending(bool local_enable)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    if (local_enable) {
        CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    } else {
        CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    }
    CSRS(CSR_MIP, AI_IBC_MIP_STIP);
}

static inline void ai_ibc_prepare_mret_to_m_with_mie_off(void)
{
    uint64_t mstatus = CSRR(CSR_MSTATUS);

    mstatus &= ~(MSTATUS_MIE | AI_IBC_MSTATUS_MPIE |
        AI_IBC_MSTATUS_MPP_MASK);
    mstatus |= AI_IBC_MSTATUS_MPP_M;
    CSRW(CSR_MSTATUS, mstatus);
}

static inline void ai_ibc_restore_m_state(uint64_t saved_mstatus,
    uint64_t saved_mie,
    uint64_t saved_mideleg,
    uint64_t saved_mtvec,
    uint64_t saved_mepc,
    uint64_t saved_menvcfg)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline bool ai_ibc_enable_stip_and_wait(void)
{
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT && !excpt.triggered; ++i) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "addi x0, x0, 0\n\t"
            ".option pop\n\t"
            ::: "memory");
    }

    return excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
}

static inline bool ai_ibc_enable_delegated_ssip_and_wait_hs(void)
{
    goto_priv(PRIV_HS);
    CSRS(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL; ++i) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "addi x0, x0, 0\n\t"
            ".option pop\n\t"
            ::: "memory");
    }

    return ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL;
}

static bool ai_ibc_vmodule_execute_trigger_delay_contract(
    uint64_t delay_cycles)
{
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t delay_tag = (delay_cycles & 0xffULL) << 8;
    const uint64_t guard_seed = 0x5152535455565700ULL | delay_tag;
    const uint64_t target_seed = 0x6162636465666700ULL | delay_tag;
    const uint64_t after_seed = 0x7172737475767700ULL | delay_tag;
    const uint64_t blocked_payload = 0x8182838485868700ULL | delay_tag;
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(true, delay_cycles);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < 8ULL; ++i) {
        marker += 1ULL;
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_store_once(target_addr, blocked_payload);
    marker += 1ULL;

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool event_contract =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_has_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;
    const bool side_effect_contract =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;
    const bool progress_contract = marker == 9ULL;
    const bool clear_source_contract = (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    return event_contract &&
        side_effect_contract &&
        progress_contract &&
        clear_source_contract;
}

static bool ai_ibc_vmodule_difftest_marker_round(uint64_t delay_cycles,
    uint64_t *order_marker)
{
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t delay_tag = (delay_cycles & 0xffULL) << 8;
    const uint64_t guard_seed = 0x6466746576670000ULL | delay_tag;
    const uint64_t target_seed = 0x6466746576740000ULL | delay_tag;
    const uint64_t after_seed = 0x6466746576610000ULL | delay_tag;
    const uint64_t blocked_payload = 0x6466746576620000ULL | delay_tag;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, delay_cycles);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, blocked_payload);

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    uint64_t first = ai_ibc_trap_cause[0];
    uint64_t second = ai_ibc_trap_cause[1];
    if (order_marker != NULL) {
        *order_marker = ((first & 0xffULL) << 8) | (second & 0xffULL);
    }

    const bool two_event_order_is_self_consistent =
        ai_ibc_trap_count == 2ULL &&
        ((first == CAUSE_BKP && second == CAUSE_MEI) ||
            (first == CAUSE_MEI && second == CAUSE_BKP));
    const bool bkp_record_is_precise =
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL;
    const bool mei_record_is_unique_and_cleared =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool side_effect_contract =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    return two_event_order_is_self_consistent &&
        bkp_record_is_precise &&
        mei_record_is_unique_and_cleared &&
        side_effect_contract;
}

bool ai_interrupt_m_stip_pending_disabled_ebreak_then_enable_interrupt_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uintptr_t marker = 0ULL;

    ai_ibc_prepare_stip_pending(false);

    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();
    marker += 1ULL;

    const bool breakpoint_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe;
    const bool pending_preserved_after_breakpoint =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;
    const bool marker_after_breakpoint = marker == 1ULL;
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("pending STIP with mie.STIE disabled does not mask the synchronous M-mode ebreak breakpoint",
        breakpoint_seen);
    TEST_ASSERT("the ebreak handler returns to the next instruction once and preserves the pending STIP",
        marker_after_breakpoint &&
        pending_preserved_after_breakpoint);
    TEST_ASSERT("after explicitly enabling STIE/MIE, the preserved pending STIP is delivered once and cleared",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_stip_pending_disabled_ebreak_then_enable_interrupt_corner);
}

bool ai_interrupt_m_vmodule_meip_pending_disabled_ebreak_then_enable_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(false, 0ULL);
    const bool meip_visible_before_breakpoint =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT);

    ai_ibc_ebreak_probe();
    marker = 0x6d656970ULL;

    const bool breakpoint_seen =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe;
    const bool meip_preserved_after_breakpoint =
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL &&
        marker == 0x6d656970ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool mei_delivered_once =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;
    const bool meip_cleared_by_handler =
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule MEIP can be held pending behind disabled MEIE/MIE before the synchronous ebreak",
        meip_visible_before_breakpoint);
    TEST_ASSERT("a VModule MEIP pending source does not mask the precise M-mode ebreak breakpoint or skip the post-breakpoint marker",
        breakpoint_seen &&
        meip_preserved_after_breakpoint);
    TEST_ASSERT("after the testcase enables MEIE/MIE, the preserved VModule MEIP is delivered once and cleared through vm_reg_0",
        mei_delivered_once &&
        meip_cleared_by_handler);

    TEST_END(ai_interrupt_m_vmodule_meip_pending_disabled_ebreak_then_enable_corner);
}

bool ai_interrupt_m_stip_pending_disabled_c_ebreak_then_enable_interrupt_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    ai_ibc_prepare_stip_pending(true);

    TEST_SETUP_EXCEPT();
    ai_ibc_cebreak_probe();
    marker = 0x51ULL;

    const bool cbreak_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_cebreak_probe;
    const bool cbreak_returned_before_interrupt =
        marker == 0x51ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("pending STIP with mie.STIE set but mstatus.MIE clear still reports c.ebreak as a synchronous breakpoint",
        cbreak_seen);
    TEST_ASSERT("mret from the compressed-breakpoint handler does not open global MIE before the testcase marker",
        cbreak_returned_before_interrupt);
    TEST_ASSERT("only the testcase's explicit global MIE enable lets the preserved STIP enter the M-mode interrupt handler",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_stip_pending_disabled_c_ebreak_then_enable_interrupt_corner);
}

bool ai_interrupt_m_stip_enabled_store_trigger_no_side_effect_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x1011121314151617ULL;
    const uint64_t target_seed = 0x2122232425262728ULL;
    const uint64_t after_seed = 0x3132333435363738ULL;
    const uint64_t blocked_payload = 0xaabbccddeeff0011ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_stip_pending(true);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool breakpoint_logged = ai_ibc_log_has_cause(CAUSE_BKP);
    const bool interrupt_logged = ai_ibc_log_has_cause(CAUSE_STI);
    const bool store_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;
    const bool pending_cleared =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending and enabled, the store-address breakpoint and timer interrupt are both observed without fixed order assumptions",
        breakpoint_logged &&
        interrupt_logged);
    TEST_ASSERT("when the store reports breakpoint in the enabled-interrupt window, its memory side effect and guards remain suppressed",
        store_suppressed);
    TEST_ASSERT("the interrupt leg clears only the pending STIP after observing it",
        pending_cleared);

    TEST_END(ai_interrupt_m_stip_enabled_store_trigger_no_side_effect_corner);
}

bool ai_interrupt_m_vmodule_delayed_mei_execute_trigger_event_set_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x1213141516171819ULL;
    const uint64_t target_seed = 0x2223242526272829ULL;
    const uint64_t after_seed = 0x3233343536373839ULL;
    const uint64_t blocked_payload = 0x4243444546474849ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(true, 32ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < 16ULL; ++i) {
        marker += 1ULL;
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_store_once(target_addr, blocked_payload);
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_has_cause(CAUSE_BKP) &&
            ai_ibc_log_has_cause(CAUSE_MEI)) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool breakpoint_logged =
        ai_ibc_log_has_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once);
    const bool mei_logged_once = ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;
    const bool store_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;
    const bool marker_progress_once = marker == 17ULL;
    const bool mei_source_cleared =
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule delayed MEI and the execute-PC breakpoint are both logged without assuming a fixed arrival order",
        breakpoint_logged &&
        mei_logged_once);
    TEST_ASSERT("the execute-PC breakpoint suppresses the target store side effect while ordinary window markers retire exactly once",
        store_suppressed &&
        marker_progress_once);
    TEST_ASSERT("the MEI handler clears the VModule source through vm_reg_0 so the external interrupt does not keep retriggering",
        mei_source_cleared);

    TEST_END(ai_interrupt_m_vmodule_delayed_mei_execute_trigger_event_set_corner);
}

bool ai_interrupt_m_stip_handler_then_store_trigger_state_preserved_corner()
{
    TEST_START();

    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t target_seed = 0x4142434445464748ULL;
    const uint64_t guard_seed = 0x5152535455565758ULL;
    const uint64_t blocked_payload = 0x6162636465666768ULL;
    const uint64_t final_payload = 0x7172737475767778ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(target_addr, target_seed);
    write64(guard_addr, guard_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);

    ai_ibc_prepare_stip_pending(true);
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();

    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, blocked_payload);
    const bool trigger_after_interrupt =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, final_payload);
    const bool trigger_clear_allows_commit =
        excpt.triggered == false &&
        read64(target_addr) == final_payload &&
        read64(guard_addr) == guard_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the enabled STIP is handled and cleared before the later trigger probe",
        interrupt_seen);
    TEST_ASSERT("after interrupt return, the still-armed store-address trigger remains live and suppresses the target store",
        trigger_after_interrupt);
    TEST_ASSERT("clearing the trigger after the interrupt path lets the same store helper commit normally",
        trigger_clear_allows_commit);

    TEST_END(ai_interrupt_m_stip_handler_then_store_trigger_state_preserved_corner);
}

bool ai_interrupt_m_vmodule_mei_handler_then_store_trigger_state_preserved_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x564d454954524731ULL;
    const uint64_t target_seed = 0x564d454954524732ULL;
    const uint64_t after_seed = 0x564d454954524733ULL;
    const uint64_t blocked_payload = 0x564d454954524734ULL;
    const uint64_t final_payload = 0x564d454954524735ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t tdata1_after_mei = 0ULL;
    uint64_t tdata2_after_mei = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const uint64_t tselect_after_mei =
        ai_ibc_read_trigger_csr_slot(0, &tdata1_after_mei,
            &tdata2_after_mei);
    const bool mei_handler_recorded_and_cleared =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_trap_cause[0] == CAUSE_MEI &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;
    const bool trigger_csr_preserved_by_mei_handler =
        tselect_after_mei == 0ULL &&
        ai_ibc_mcontrol6_type(tdata1_after_mei) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (tdata1_after_mei & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        tdata2_after_mei == target_addr;

    const uint64_t trap_count_after_mei = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, blocked_payload);
    const bool breakpoint_index_valid =
        trap_count_after_mei < AI_IBC_LOG_SLOTS;
    const uint64_t breakpoint_cause =
        breakpoint_index_valid ? ai_ibc_trap_cause[trap_count_after_mei] : 0ULL;
    const uint64_t breakpoint_epc =
        breakpoint_index_valid ? ai_ibc_trap_epc[trap_count_after_mei] : 0ULL;
    const uint64_t breakpoint_tval =
        breakpoint_index_valid ? ai_ibc_trap_tval[trap_count_after_mei] : 0ULL;
    const bool trigger_after_mei_return =
        ai_ibc_trap_count == trap_count_after_mei + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        breakpoint_cause == CAUSE_BKP &&
        breakpoint_epc == (uintptr_t)ai_ibc_store_once &&
        breakpoint_tval == target_addr &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_breakpoint = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, final_payload);
    const bool trigger_clear_allows_commit =
        ai_ibc_trap_count == trap_count_after_breakpoint &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the VModule MEI handler records exactly one external interrupt, clears vm_reg_0 source, and leaves memory untouched",
        mei_handler_recorded_and_cleared);
    TEST_ASSERT("returning from the VModule MEI handler preserves the armed store-address trigger CSR tuple",
        trigger_csr_preserved_by_mei_handler);
    TEST_ASSERT("after the VModule interrupt return, the same target store still reports breakpoint and suppresses its payload",
        trigger_after_mei_return);
    TEST_ASSERT("only an explicit trigger clear lets the post-interrupt control store commit without late MEI or ghost breakpoint",
        trigger_clear_allows_commit);

    TEST_END(ai_interrupt_m_vmodule_mei_handler_then_store_trigger_state_preserved_corner);
}

bool ai_interrupt_m_stip_pending_execute_trigger_then_access_fault_then_interrupt_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uintptr_t bad_addr = AI_IBC_BAD_PADDR;

    goto_priv(PRIV_M);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_load_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    TEST_SETUP_EXCEPT();
    (void)ai_ibc_load_once(bad_addr);
    const bool breakpoint_before_fault =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    TEST_SETUP_EXCEPT();
    (void)ai_ibc_load_once(bad_addr);
    const bool fault_after_trigger_clear =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_LAF &&
        excpt.tval == bad_addr &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but mie.STIE disabled, an execute-PC trigger on an access-faulting M-mode load reports breakpoint first and keeps STIP pending",
        breakpoint_before_fault);
    TEST_ASSERT("after clearing the trigger while STIP is still disabled, the same load reports load access fault with the raw address in mtval",
        fault_after_trigger_clear);
    TEST_ASSERT("after breakpoint and access-fault handlers both return, the preserved STIP pending is still delivered once when enabled",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_stip_pending_execute_trigger_then_access_fault_then_interrupt_corner);
}

bool ai_interrupt_m_stip_pending_chain_mismatch_then_interrupt_then_chain_closed_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x8182838485868788ULL;
    const uint64_t target_seed = 0x9192939495969798ULL;
    const uint64_t after_seed = 0xa1a2a3a4a5a6a7a8ULL;
    const uint64_t mismatch_payload = 0xb1b2b3b4b5b6b7b8ULL;
    const uint64_t blocked_payload = 0xc1c2c3c4c5c6c7c8ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        true);
    ai_ibc_arm_mcontrol6_slot(1, guard_addr, AI_IBC_MCONTROL6_STORE,
        false);

    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, mismatch_payload);
    const bool mismatch_chain_does_not_break =
        excpt.triggered == false &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == mismatch_payload &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        true);
    ai_ibc_arm_mcontrol6_slot(1, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, blocked_payload);
    const bool closed_chain_breaks_and_suppresses =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == mismatch_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with slot0 chained to a nonmatching store slot1, pending-disabled STIP does not turn the half-closed chain into a breakpoint",
        mismatch_chain_does_not_break);
    TEST_ASSERT("after the chain-mismatch commit, explicitly enabling STIP delivers the preserved interrupt once",
        interrupt_seen);
    TEST_ASSERT("after closing the two-slot store chain, the same target store reports breakpoint and preserves the committed image",
        closed_chain_breaks_and_suppresses);

    TEST_END(ai_interrupt_m_stip_pending_chain_mismatch_then_interrupt_then_chain_closed_corner);
}

bool ai_interrupt_m_stip_enable_csr_gap_ebreak_then_global_enable_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;
    uint64_t mie_snapshot = 0ULL;
    uint64_t mip_snapshot = 0ULL;

    ai_ibc_prepare_stip_pending(false);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    mie_snapshot = CSRR(CSR_MIE);
    mip_snapshot = CSRR(CSR_MIP);

    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();
    marker = 0x5aULL;

    const bool breakpoint_after_local_enable =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        marker == 0x5aULL &&
        (mie_snapshot & AI_IBC_MIE_STIE) != 0ULL &&
        (mip_snapshot & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("setting mie.STIE while mstatus.MIE remains clear leaves the pending STIP behind the synchronous ebreak",
        breakpoint_after_local_enable);
    TEST_ASSERT("only the later global MIE enable delivers the pending STIP, proving the breakpoint xRET path did not open interrupts early",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_stip_enable_csr_gap_ebreak_then_global_enable_corner);
}

bool ai_interrupt_m_mret_target_ebreak_pending_stip_pc_precise_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t marker = 0ULL;

    ai_ibc_prepare_stip_pending(false);
    ai_ibc_prepare_mret_to_m_with_mie_off();

    TEST_SETUP_EXCEPT();
    ai_ibc_mret_to_ebreak_probe(&marker);

    const bool breakpoint_at_mret_target =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_mret_ebreak_target;
    const bool marker_after_target_breakpoint =
        marker == 0x6d726574ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("mret returns to the ebreak target PC before the pending-disabled STIP can redirect control",
        breakpoint_at_mret_target);
    TEST_ASSERT("the breakpoint target continuation runs once while the pending STIP remains held behind global MIE",
        marker_after_target_breakpoint);
    TEST_ASSERT("after the testcase enables interrupts, the same preserved STIP is delivered and cleared",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_mret_target_ebreak_pending_stip_pc_precise_corner);
}

bool ai_interrupt_m_stip_csr_write_window_ebreak_preserves_pending_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE | MIE_MEIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);

    for (uint64_t i = 0; i < AI_IBC_STIP_WRITE_ROUNDS / 2ULL; ++i) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        marker += 1ULL;
    }

    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();

    const bool breakpoint_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe;
    const bool stip_pending_after_breakpoint =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    for (uint64_t i = 0; i < AI_IBC_STIP_WRITE_ROUNDS / 2ULL; ++i) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        marker += 1ULL;
    }

    const uint64_t final_mip = CSRR(CSR_MIP);
    const bool no_interrupt_taken = excpt.triggered &&
        excpt.cause == CAUSE_BKP;
    const bool csr_write_window_preserved =
        marker == AI_IBC_STIP_WRITE_ROUNDS &&
        (final_mip & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the ebreak inserted into a repeated mip.STIP CSR write window is reported as a breakpoint",
        breakpoint_seen);
    TEST_ASSERT("the breakpoint handler does not clear the STIP bit written before the breakpoint",
        stip_pending_after_breakpoint);
    TEST_ASSERT("with STIE/MEIE disabled, the loop completes without interrupt entry and the final STIP pending bit remains visible",
        no_interrupt_taken &&
        csr_write_window_preserved);

    TEST_END(ai_interrupt_m_stip_csr_write_window_ebreak_preserves_pending_corner);
}

bool ai_interrupt_m_vmodule_meip_stip_write_window_ebreak_preserves_both_pending_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;
    uint64_t observed_mip_before_breakpoint = 0ULL;
    uint64_t observed_mip_after_breakpoint = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE | MIE_MEIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_prepare_vmodule_meip_pending(false, 64ULL);

    const uint64_t rounds =
        ai_ibc_write_stip_until_meip_pending(
            AI_IBC_VMODULE_POLL_LIMIT,
            &observed_mip_before_breakpoint);

    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();
    marker = 0x73746970ULL;
    observed_mip_after_breakpoint = CSRR(CSR_MIP);

    for (uint64_t i = 0; i < AI_IBC_STIP_WRITE_ROUNDS; ++i) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        marker += 1ULL;
    }

    const uint64_t final_mip = CSRR(CSR_MIP);
    const bool both_pending_before_breakpoint =
        rounds > 0ULL &&
        rounds < AI_IBC_VMODULE_POLL_LIMIT &&
        (observed_mip_before_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (observed_mip_before_breakpoint & MIP_MEIP) != 0ULL;
    const bool breakpoint_preserved_both_pending =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe &&
        (observed_mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (observed_mip_after_breakpoint & MIP_MEIP) != 0ULL &&
        marker == (0x73746970ULL + AI_IBC_STIP_WRITE_ROUNDS);
    const bool no_interrupt_taken_while_disabled =
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL &&
        (final_mip & AI_IBC_MIP_STIP) != 0ULL &&
        (final_mip & MIP_MEIP) != 0ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule MEIP pending becomes visible while repeated CSR writes keep mip.STIP pending with both interrupt enables closed",
        both_pending_before_breakpoint);
    TEST_ASSERT("the ebreak inside the STIP CSR write window reports breakpoint and preserves both CSR STIP and VModule MEIP pending bits",
        breakpoint_preserved_both_pending);
    TEST_ASSERT("with MEIE/STIE and global MIE closed, neither pending source enters the interrupt handler during the write window",
        no_interrupt_taken_while_disabled);

    TEST_END(ai_interrupt_m_vmodule_meip_stip_write_window_ebreak_preserves_both_pending_corner);
}

bool ai_interrupt_m_vmodule_meip_back_to_back_clear_source_reinject_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(false, 0ULL);
    const bool meip_held_before_breakpoint =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT);

    ai_ibc_ebreak_probe();
    marker += 1ULL;

    const bool breakpoint_only_phase =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mip[0] & MIP_MEIP) != 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL &&
        marker == 1ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const uint64_t mei_count_after_first = ai_ibc_log_count_cause(CAUSE_MEI);
    const bool first_mei_cleared_source =
        mei_count_after_first == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_level_sensitive_repeat =
        ai_ibc_log_count_cause(CAUSE_MEI) == mei_count_after_first &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool reinject_delivered_once =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule MEIP stays pending behind disabled MEIE/MIE and the breakpoint handler only snapshots it",
        meip_held_before_breakpoint &&
        breakpoint_only_phase);
    TEST_ASSERT("the first MEI handler clears the level-sensitive VModule source instead of relying on CSR pending writes",
        first_mei_cleared_source &&
        no_level_sensitive_repeat);
    TEST_ASSERT("reinjecting vm_reg_0.MEIP after source clear delivers one fresh MEI without permanently masking the path",
        reinject_delivered_once);

    TEST_END(ai_interrupt_m_vmodule_meip_back_to_back_clear_source_reinject_corner);
}

bool ai_interrupt_m_vmodule_meip_chain_reprogram_no_stale_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t miss_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x2c6861696e670001ULL;
    const uint64_t miss_seed = 0x2c6861696e6d0002ULL;
    const uint64_t target_seed = 0x2c6861696e740003ULL;
    const uint64_t after_seed = 0x2c6861696e610004ULL;
    const uint64_t mismatch_payload = 0x2c6861696e780005ULL;
    const uint64_t target_payload = 0x2c6861696e790006ULL;
    const uint64_t blocked_payload = 0x2c6861696e7a0007ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(miss_addr, miss_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_vmodule_meip_pending(true, 32ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_arm_mcontrol6_slot(2, target_addr, AI_IBC_MCONTROL6_STORE,
        true);
    ai_ibc_arm_mcontrol6_slot(3, guard_addr, AI_IBC_MCONTROL6_STORE,
        false);
    for (uint64_t i = 0; i < 8ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_store_once(target_addr, mismatch_payload);

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mismatch_config_survives_mei =
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == miss_seed &&
        read64(target_addr) == mismatch_payload &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_arm_mcontrol6_slot(2, target_addr, AI_IBC_MCONTROL6_STORE,
        true);
    ai_ibc_arm_mcontrol6_slot(3, miss_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_store_once(target_addr, target_payload);

    const bool reprogrammed_mismatch_no_stale_closed =
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == miss_seed &&
        read64(target_addr) == target_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_arm_mcontrol6_slot(2, target_addr, AI_IBC_MCONTROL6_STORE,
        true);
    ai_ibc_arm_mcontrol6_slot(3, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_store_once(target_addr, blocked_payload);

    const bool closed_chain_breaks_after_reprogram =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == miss_seed &&
        read64(target_addr) == target_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("while VModule MEIP is delivered in the slot2/slot3 chain programming window, the initial mismatch chain commits without ghost breakpoint",
        mismatch_config_survives_mei);
    TEST_ASSERT("after interrupt return, reprogramming slot3 to a different mismatch address does not retain stale closed-chain state",
        reprogrammed_mismatch_no_stale_closed);
    TEST_ASSERT("after reprogramming the same two-slot chain closed, the target store reports one breakpoint and its side effect is suppressed",
        closed_chain_breaks_after_reprogram);

    TEST_END(ai_interrupt_m_vmodule_meip_chain_reprogram_no_stale_corner);
}

bool ai_interrupt_m_vmodule_meip_execute_trigger_delay_sweep_event_contract_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    const bool delay_0_contract =
        ai_ibc_vmodule_execute_trigger_delay_contract(0ULL);
    const bool delay_1_contract =
        ai_ibc_vmodule_execute_trigger_delay_contract(1ULL);
    const bool delay_16_contract =
        ai_ibc_vmodule_execute_trigger_delay_contract(16ULL);

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with vm_reg_8 delay 0, VModule MEI plus execute-PC breakpoint satisfies the event-set and side-effect contract without fixed-order assumptions",
        delay_0_contract);
    TEST_ASSERT("with vm_reg_8 delay 1, the same VModule/breakpoint contract remains stable without assuming single-cycle handler timing",
        delay_1_contract);
    TEST_ASSERT("with vm_reg_8 delay 16, delayed MEI is still observed once, clears source, and does not disturb the breakpoint-suppressed store",
        delay_16_contract);

    TEST_END(ai_interrupt_m_vmodule_meip_execute_trigger_delay_sweep_event_contract_corner);
}

bool ai_interrupt_m_vmodule_bkp_mei_back_to_back_difftest_marker_contract_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t delay0_order = 0ULL;
    uint64_t delay3_order = 0ULL;

    const bool delay0_marker_contract =
        ai_ibc_vmodule_difftest_marker_round(0ULL, &delay0_order);
    const bool delay3_marker_contract =
        ai_ibc_vmodule_difftest_marker_round(3ULL, &delay3_order);
    const bool order_markers_are_decodable =
        (delay0_order == ((CAUSE_BKP & 0xffULL) << 8 |
            (CAUSE_MEI & 0xffULL)) ||
            delay0_order == ((CAUSE_MEI & 0xffULL) << 8 |
                (CAUSE_BKP & 0xffULL))) &&
        (delay3_order == ((CAUSE_BKP & 0xffULL) << 8 |
            (CAUSE_MEI & 0xffULL)) ||
            delay3_order == ((CAUSE_MEI & 0xffULL) << 8 |
                (CAUSE_BKP & 0xffULL)));

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("delay0 VModule MEI plus execute breakpoint leaves a two-entry handler log whose order marker can be compared against LinkNan commit/difftest trap events without assuming a fixed winner",
        delay0_marker_contract &&
        order_markers_are_decodable);
    TEST_ASSERT("delay3 repeats the back-to-back BP/MEI stimulus with fresh handler log entries, proving the previous PC/cause record is not reused across difftest-marker rounds",
        delay3_marker_contract &&
        order_markers_are_decodable);

    TEST_END(ai_interrupt_m_vmodule_bkp_mei_back_to_back_difftest_marker_contract_corner);
}

bool ai_interrupt_m_vmodule_redirect_trigger_metadata_fresh_pc_va_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t first_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t second_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uintptr_t first_store_pc = (uintptr_t)ai_ibc_store_once;
    const uintptr_t second_store_pc =
        (uintptr_t)ai_ibc_chain_exec_store_store_site;
    const uint64_t guard_seed = 0x72646d6574676401ULL;
    const uint64_t first_seed = 0x72646d6576613102ULL;
    const uint64_t second_seed = 0x72646d6576613203ULL;
    const uint64_t after_seed = 0x72646d6561667404ULL;
    const uint64_t first_blocked = 0x72646d65626c3110ULL;
    const uint64_t first_committed = 0x72646d65636d3120ULL;
    const uint64_t second_blocked = 0x72646d65626c3230ULL;
    const uint64_t second_committed = 0x72646d65636d3240ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(first_addr, first_seed);
    write64(second_addr, second_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, first_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(first_addr, first_blocked);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool first_episode_metadata_is_fresh =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, first_store_pc,
            first_addr) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, second_store_pc) == 0ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, second_addr) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(first_addr) == first_seed &&
        read64(second_addr) == second_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_first_clear = ai_ibc_trap_count;
    ai_ibc_store_once(first_addr, first_committed);
    const bool first_clear_allows_current_path_commit =
        ai_ibc_trap_count == trap_count_after_first_clear &&
        read64(guard_addr) == guard_seed &&
        read64(first_addr) == first_committed &&
        read64(second_addr) == second_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_arm_mcontrol6_slot(0, second_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_chain_exec_store_probe((uint64_t *)second_addr,
        second_blocked);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool second_episode_metadata_refreshes_pc_va =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, first_store_pc,
            first_addr) == 1ULL &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, second_store_pc,
            second_addr) == 1ULL &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, first_store_pc,
            second_addr) == 0ULL &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, second_store_pc,
            first_addr) == 0ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, first_addr) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, second_addr) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(first_addr) == first_committed &&
        read64(second_addr) == second_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_second_clear = ai_ibc_trap_count;
    ai_ibc_chain_exec_store_probe((uint64_t *)second_addr,
        second_committed);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool final_clear_has_no_late_redirect_metadata =
        ai_ibc_trap_count == trap_count_after_second_clear &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(first_addr) == first_committed &&
        read64(second_addr) == second_committed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI overlapping the first store-address trigger, the breakpoint log binds cause, epc, and tval to the first helper and first VA only",
        first_episode_metadata_is_fresh);
    TEST_ASSERT("after clearing that trigger/source, the first helper commits on the current path exactly once without reusing stale redirect metadata",
        first_clear_allows_current_path_commit);
    TEST_ASSERT("after rearming on a different store PC and VA, the second breakpoint refreshes both epc and tval instead of mixing either field with the previous episode",
        second_episode_metadata_refreshes_pc_va);
    TEST_ASSERT("after final trigger/source clear, the second helper commits once and no late MEI or ghost breakpoint replays old metadata",
        final_clear_has_no_late_redirect_metadata);

    TEST_END(ai_interrupt_m_vmodule_redirect_trigger_metadata_fresh_pc_va_corner);
}

bool ai_interrupt_m_vmodule_nmi31_breakpoint_csr_owner_no_cross_write_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t guard_seed = 0x726e6d6933316701ULL;
    const uint64_t target_seed = 0x726e6d6933317402ULL;
    const uint64_t after_seed = 0x726e6d6933316103ULL;
    const uint64_t blocked_payload = 0x726e6d6933316210ULL;
    const uint64_t committed_payload = 0x726e6d6933316320ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t mcause_after_episode = 0ULL;
    uint64_t mepc_after_episode = 0ULL;
    uint64_t mncause_after_episode = 0ULL;
    uint64_t mnepc_after_episode = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_store_once(target_addr, blocked_payload);

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    mcause_after_episode = CSRR(CSR_MCAUSE);
    mepc_after_episode = CSRR(CSR_MEPC);
    mncause_after_episode = CSRR(CSR_MNCAUSE);
    mnepc_after_episode = CSRR(CSR_MNEPC);

    const bool rnmi_record_keeps_mn_owner =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mncause[0] != CAUSE_BKP &&
        mncause_after_episode == AI_IBC_NMI31_CAUSE &&
        mnepc_after_episode != 0ULL;
    const bool breakpoint_record_keeps_m_owner =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 1ULL &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_trap_epc[0] == store_pc &&
        mcause_after_episode == CAUSE_BKP &&
        mepc_after_episode != ai_ibc_rnmi_mnepc[0];
    const bool return_paths_are_exactly_once =
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL;
    const bool breakpoint_side_effect_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool nmi_source_clear_no_repeat =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_store_once(target_addr, committed_payload);
    const bool clear_state_allows_plain_store =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule vm_reg_9 NMI_31 writes the RNMI record through mncause/mnepc while the execute breakpoint writes only the ordinary mcause/mepc log",
        rnmi_record_keeps_mn_owner &&
        breakpoint_record_keeps_m_owner);
    TEST_ASSERT("the mixed RNMI/breakpoint episode returns through exactly one mnret and one mret, without reusing the wrong CSR owner for either trap",
        return_paths_are_exactly_once);
    TEST_ASSERT("the execute breakpoint still suppresses the target store side effect, then clearing the trigger and NMI source lets one plain store commit",
        breakpoint_side_effect_suppressed &&
        nmi_source_clear_no_repeat &&
        clear_state_allows_plain_store);

    TEST_END(ai_interrupt_m_vmodule_nmi31_breakpoint_csr_owner_no_cross_write_corner);
}

bool ai_interrupt_m_vmodule_rnmi_tdata2_half_config_no_ghost_breakpoint_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    const bool rnmi_after_tselect_only =
        ai_ibc_rnmi_trigger_half_config_round(false, 0x11ULL);
    const bool rnmi_after_tdata2_only =
        ai_ibc_rnmi_trigger_half_config_round(true, 0x22ULL);

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule NMI_31 interrupts after tselect is chosen but before tdata1/tdata2 programming, no half-selected trigger produces a ghost breakpoint",
        rnmi_after_tselect_only);
    TEST_ASSERT("when RNMI interrupts after tdata2 is written while tdata1 remains cleared, the half-configured address does not arm until the final mcontrol6 tdata1 write",
        rnmi_after_tdata2_only);

    TEST_END(ai_interrupt_m_vmodule_rnmi_tdata2_half_config_no_ghost_breakpoint_corner);
}

bool ai_interrupt_m_vmodule_meip_tselect_tdata_half_config_no_ghost_breakpoint_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    const bool tselect_only_interrupt_contract =
        ai_ibc_vmodule_meip_trigger_half_config_round(false, 0x41ULL);
    const bool tdata2_interrupt_contract =
        ai_ibc_vmodule_meip_trigger_half_config_round(true, 0x52ULL);

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule MEI interrupts after tselect is switched but before tdata2/tdata1 are programmed, slot0 remains disabled and the target store commits without a ghost breakpoint",
        tselect_only_interrupt_contract);
    TEST_ASSERT("when VModule MEI interrupts after tdata2 is written while tdata1 is still cleared, the half-configured address stays inert until the final mcontrol6 tdata1 write",
        tdata2_interrupt_contract);

    TEST_END(ai_interrupt_m_vmodule_meip_tselect_tdata_half_config_no_ghost_breakpoint_corner);
}

bool ai_interrupt_m_vmodule_exceptionbuffer_metadata_same_source_corner()
{
    TEST_START();

    const uintptr_t elem1_addr =
        (uintptr_t)&ai_ibc_vec_dst[1];
    const uintptr_t elem2_addr =
        (uintptr_t)&ai_ibc_vec_dst[2];
    const uint64_t src0_seed = 0x6578666d64733001ULL;
    const uint64_t src1_payload = 0x6578666d64733110ULL;
    const uint64_t src2_payload = 0x6578666d64733220ULL;
    const uint64_t src3_seed = 0x6578666d64733303ULL;
    const uint64_t dst0_seed = 0x6578666d64743001ULL;
    const uint64_t dst1_seed = 0x6578666d64743102ULL;
    const uint64_t dst2_seed = 0x6578666d64743203ULL;
    const uint64_t dst3_seed = 0x6578666d64743304ULL;
    const uint64_t control1_payload = 0x6578666d63743111ULL;
    const uint64_t control2_payload = 0x6578666d63743222ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    ai_ibc_enable_vector_state();
    ai_ibc_vec_src[0] = src0_seed;
    ai_ibc_vec_src[1] = src1_payload;
    ai_ibc_vec_src[2] = src2_payload;
    ai_ibc_vec_src[3] = src3_seed;
    ai_ibc_vec_dst[0] = dst0_seed;
    ai_ibc_vec_dst[1] = dst1_seed;
    ai_ibc_vec_dst[2] = dst2_seed;
    ai_ibc_vec_dst[3] = dst3_seed;
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(1ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_LOG_VSTART;

    ai_ibc_arm_mcontrol6_slot(0, elem1_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool first_episode_same_source =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_vector_store64_site) == 1ULL &&
        ai_ibc_log_count_bkp_tval_vstart(elem1_addr, 1ULL) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_seed &&
        ai_ibc_vec_dst[1] == dst1_seed &&
        ai_ibc_vec_dst[2] == dst2_seed &&
        ai_ibc_vec_dst[3] == dst3_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    CSRW(CSR_VSTART, 0ULL);
    const uint64_t trap_count_after_first_clear = ai_ibc_trap_count;
    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    const bool first_clear_replays_from_start =
        ai_ibc_trap_count == trap_count_after_first_clear &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_seed &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == src2_payload &&
        ai_ibc_vec_dst[3] == src3_seed;

    ai_ibc_vec_src[1] = control1_payload;
    ai_ibc_vec_src[2] = control2_payload;
    ai_ibc_vec_dst[0] = dst0_seed;
    ai_ibc_vec_dst[1] = dst1_seed;
    ai_ibc_vec_dst[2] = dst2_seed;
    ai_ibc_vec_dst[3] = dst3_seed;
    CSRW(CSR_VSTART, 0ULL);
    ai_ibc_arm_mcontrol6_slot(0, elem2_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool second_episode_refreshes_metadata =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_vector_store64_site) == 2ULL &&
        ai_ibc_log_count_bkp_tval_vstart(elem1_addr, 1ULL) == 1ULL &&
        ai_ibc_log_count_bkp_tval_vstart(elem2_addr, 2ULL) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_seed &&
        ai_ibc_vec_dst[1] == control1_payload &&
        ai_ibc_vec_dst[2] == dst2_seed &&
        ai_ibc_vec_dst[3] == dst3_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    CSRW(CSR_VSTART, 0ULL);
    const uint64_t trap_count_after_second_clear = ai_ibc_trap_count;
    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    const bool final_clear_replays_all_lanes_once =
        ai_ibc_trap_count == trap_count_after_second_clear &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_seed &&
        ai_ibc_vec_dst[1] == control1_payload &&
        ai_ibc_vec_dst[2] == control2_payload &&
        ai_ibc_vec_dst[3] == src3_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_ghost_after_sources_clear =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        CSRR(CSR_VSTART) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_VSTART, 0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule MEI overlaps the first vector-store address breakpoint, the breakpoint log keeps cause, tval, and vstart tied to element1 while later lanes stay old",
        first_episode_same_source);
    TEST_ASSERT("after clearing the trigger and resetting vstart, the same vector store replays from element0 through the suffix without an extra trap",
        first_clear_replays_from_start);
    TEST_ASSERT("after rearming the trigger on element2 and reinjecting MEI, breakpoint metadata refreshes to element2 instead of reusing the stale element1 tval/vstart pair",
        second_episode_refreshes_metadata);
    TEST_ASSERT("final trigger/source clear replays the vector store once and leaves no ghost breakpoint, MEI, or nonzero vstart state",
        final_clear_replays_all_lanes_once &&
        no_ghost_after_sources_clear);

    TEST_END(ai_interrupt_m_vmodule_exceptionbuffer_metadata_same_source_corner);
}

bool ai_interrupt_m_vmodule_vector_vstart_pending_mei_metadata_preserved_corner()
{
    TEST_START();

    const uintptr_t elem2_addr = (uintptr_t)&ai_ibc_vec_dst[2];
    const uint64_t src0_payload = 0x7673746172743001ULL;
    const uint64_t src1_payload = 0x7673746172743111ULL;
    const uint64_t src2_payload = 0x7673746172743222ULL;
    const uint64_t src3_payload = 0x7673746172743333ULL;
    const uint64_t dst0_seed = 0x7673746172643001ULL;
    const uint64_t dst1_seed = 0x7673746172643102ULL;
    const uint64_t dst2_seed = 0x7673746172643203ULL;
    const uint64_t dst3_seed = 0x7673746172643304ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_enable_vector_state();
    ai_ibc_vec_src[0] = src0_payload;
    ai_ibc_vec_src[1] = src1_payload;
    ai_ibc_vec_src[2] = src2_payload;
    ai_ibc_vec_src[3] = src3_payload;
    ai_ibc_vec_dst[0] = dst0_seed;
    ai_ibc_vec_dst[1] = dst1_seed;
    ai_ibc_vec_dst[2] = dst2_seed;
    ai_ibc_vec_dst[3] = dst3_seed;
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_LOG_VSTART;
    ai_ibc_arm_mcontrol6_slot(0, elem2_addr, AI_IBC_MCONTROL6_STORE,
        false);

    ai_ibc_prepare_vmodule_meip_pending(false, 0ULL);
    const bool meip_pending_but_masked =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT) &&
        (CSRR(CSR_MIE) & MIE_MEIE) == 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    marker += 1ULL;
    const bool breakpoint_records_vector_metadata_before_mei =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_vector_store64_site &&
        ai_ibc_log_count_bkp_tval_vstart(elem2_addr, 2ULL) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == dst2_seed &&
        ai_ibc_vec_dst[3] == dst3_seed &&
        marker == 1ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mei_delivery_does_not_overwrite_bkp_metadata =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        ai_ibc_log_count_bkp_tval_vstart(elem2_addr, 2ULL) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == dst2_seed &&
        ai_ibc_vec_dst[3] == dst3_seed &&
        marker == 1ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRW(CSR_VSTART, 0ULL);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_vector_store64_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    marker += 1ULL;
    const bool clear_trigger_replays_vector_from_start =
        ai_ibc_trap_count == trap_count_after_clear &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == src2_payload &&
        ai_ibc_vec_dst[3] == src3_payload &&
        marker == 2ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_vector_or_mei_replay =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        CSRR(CSR_VSTART) == 0ULL &&
        marker == 2ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_VSTART, 0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEIP held pending but masked, the vector store-address breakpoint records element2 tval/vstart before any interrupt handler runs",
        meip_pending_but_masked &&
        breakpoint_records_vector_metadata_before_mei);
    TEST_ASSERT("after MEIE/MIE is opened, the VModule MEI is delivered once and does not overwrite the earlier breakpoint tval/vstart or vector side-effect image",
        mei_delivery_does_not_overwrite_bkp_metadata);
    TEST_ASSERT("after clearing the trigger and vstart, the vector store replays from element0 through element3 and no late breakpoint or MEI repeats",
        clear_trigger_replays_vector_from_start &&
        no_late_vector_or_mei_replay);

    TEST_END(ai_interrupt_m_vmodule_vector_vstart_pending_mei_metadata_preserved_corner);
}

bool ai_interrupt_m_cross_page_store_execute_trigger_stip_no_partial_side_effect_corner()
{
    TEST_START();

    const enum test_page low_page = VSRW_GRW;
    const enum test_page high_page = VSRW_GRX;
    const uintptr_t split_addr = phys_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t low_phys = phys_page_base(low_page) +
        PAGE_SIZE - sizeof(uint64_t);
    const uintptr_t high_phys = phys_page_base(high_page);
    const uintptr_t low_adjacent_phys = low_phys - sizeof(uint64_t);
    const uintptr_t high_adjacent_phys = high_phys + sizeof(uint64_t);
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t low_adjacent_seed = 0x637073706c6f6101ULL;
    const uint64_t low_seed = 0x637073706c6f7702ULL;
    const uint64_t high_seed = 0x6370737068696703ULL;
    const uint64_t high_adjacent_seed = 0x6370737068696104ULL;
    const uint64_t blocked_payload = 0x63707370626c6b05ULL;
    const uint64_t committed_payload = 0x0123456789abcdefULL;
    const uint64_t expected_low_after_commit =
        (low_seed & 0xffffffffULL) |
        ((committed_payload & 0xffffffffULL) << 32U);
    const uint64_t expected_high_after_commit =
        (high_seed & 0xffffffff00000000ULL) |
        ((committed_payload >> 32U) & 0xffffffffULL);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(low_adjacent_phys, low_adjacent_seed);
    write64(low_phys, low_seed);
    write64(high_phys, high_seed);
    write64(high_adjacent_phys, high_adjacent_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t setup_mip = CSRR(CSR_MIP);
    ai_ibc_store_once(split_addr, blocked_payload);
    marker += 1ULL;
    const uint64_t marker_after_breakpoint = marker;
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const uint64_t low_after_breakpoint = read64(low_phys);
    const uint64_t high_after_breakpoint = read64(high_phys);
    uint64_t tdata1_after_breakpoint = 0ULL;
    uint64_t tdata2_after_breakpoint = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_breakpoint,
        &tdata2_after_breakpoint);

    const bool selected_pages_are_adjacent =
        phys_page_base(high_page) == phys_page_base(low_page) + PAGE_SIZE;
    const bool breakpoint_blocks_both_split_halves =
        selected_pages_are_adjacent &&
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == store_pc &&
        (ai_ibc_trap_tval[0] == 0ULL ||
            ai_ibc_trap_tval[0] == store_pc) &&
        ai_ibc_trap_magic[0] == AI_IBC_NESTED_LOG_MAGIC_BASE &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (setup_mip & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        low_after_breakpoint == low_seed &&
        high_after_breakpoint == high_seed &&
        read64(low_adjacent_phys) == low_adjacent_seed &&
        read64(high_adjacent_phys) == high_adjacent_seed &&
        marker_after_breakpoint == 1ULL;
    const bool trigger_tuple_survives_breakpoint =
        ai_ibc_mcontrol6_type(tdata1_after_breakpoint) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_EXECUTE) != 0ULL &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_MSU) ==
            AI_IBC_MCONTROL6_MSU &&
        tdata2_after_breakpoint == store_pc;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);
    marker += 1ULL;
    const uint64_t marker_after_interrupt = marker;
    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    const bool stip_delivered_once =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_trap_magic[1] == (AI_IBC_NESTED_LOG_MAGIC_BASE + 1ULL) &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        read64(low_phys) == low_seed &&
        read64(high_phys) == high_seed &&
        marker_after_interrupt == 2ULL;

    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(split_addr, committed_payload);
    marker += 1ULL;
    const uint64_t marker_after_final_store = marker;
    const bool final_store_fault_free = !excpt.triggered;
    excpt.testing = false;

    const bool final_split_store_commits_once =
        final_store_fault_free &&
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(low_adjacent_phys) == low_adjacent_seed &&
        read64(low_phys) == expected_low_after_commit &&
        read64(high_phys) == expected_high_after_commit &&
        read64(high_adjacent_phys) == high_adjacent_seed &&
        marker_after_final_store == 3ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but disabled, an execute breakpoint on the 4B+4B cross-page split store suppresses both low and high page side effects",
        breakpoint_blocks_both_split_halves,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx magic0=%lx setup_mip=%lx mip=%lx low=%lx high=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_tval[0], ai_ibc_trap_magic[0],
        setup_mip, mip_after_breakpoint,
        low_after_breakpoint, high_after_breakpoint,
        marker_after_breakpoint);
    TEST_ASSERT("the cross-page breakpoint does not consume pending STIP or rewrite the MSU execute trigger tuple",
        trigger_tuple_survives_breakpoint,
        "tdata1=%lx tdata2=%lx store_pc=%lx",
        tdata1_after_breakpoint, tdata2_after_breakpoint, store_pc);
    TEST_ASSERT("after clearing the trigger and opening STIE/MIE, the preserved STIP runs exactly once and does not create a delayed low/high half store",
        stip_delivered_once,
        "count=%lx cause1=%lx magic1=%lx mip1=%lx mip_after=%lx low=%lx high=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1],
        ai_ibc_trap_magic[1], ai_ibc_trap_mip[1],
        mip_after_interrupt, read64(low_phys), read64(high_phys),
        marker_after_interrupt);
    TEST_ASSERT("with the trigger and pending interrupt both cleared, retrying the same split store commits the expected 4B low-page tail and 4B high-page head exactly once",
        final_split_store_commits_once,
        "low=%lx expected_low=%lx high=%lx expected_high=%lx marker=%lx",
        read64(low_phys), expected_low_after_commit, read64(high_phys),
        expected_high_after_commit, marker_after_final_store);

    TEST_END(ai_interrupt_m_cross_page_store_execute_trigger_stip_no_partial_side_effect_corner);
}

bool ai_interrupt_m_vmodule_vsetvl_breakpoint_mei_final_vector_metadata_corner()
{
    TEST_START();

    const uintptr_t elem2_addr = (uintptr_t)&ai_ibc_vec_dst[2];
    const uint64_t final_vl = 4ULL;
    const uint64_t src0_payload = 0x7674797065300001ULL;
    const uint64_t src1_payload = 0x7674797065310011ULL;
    const uint64_t src2_payload = 0x7674797065320022ULL;
    const uint64_t src3_payload = 0x7674797065330033ULL;
    const uint64_t dst0_seed = 0x7674797065640001ULL;
    const uint64_t dst1_seed = 0x7674797065640102ULL;
    const uint64_t dst2_seed = 0x7674797065640203ULL;
    const uint64_t dst3_seed = 0x7674797065640304ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t old_vtype_snapshot = 0ULL;
    uint64_t final_vtype_snapshot = 0ULL;
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_enable_vector_state();
    ai_ibc_vec_src[0] = src0_payload;
    ai_ibc_vec_src[1] = src1_payload;
    ai_ibc_vec_src[2] = src2_payload;
    ai_ibc_vec_src[3] = src3_payload;
    ai_ibc_vec_dst[0] = dst0_seed;
    ai_ibc_vec_dst[1] = dst1_seed;
    ai_ibc_vec_dst[2] = dst2_seed;
    ai_ibc_vec_dst[3] = dst3_seed;
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_LOG_VSTART;

    asm volatile(
        "li t0, 2\n\t"
        "vsetvli t0, t0, e32, m1, ta, ma\n\t"
        ::: "t0", "memory");
    const uint64_t old_vl_snapshot = CSRR(CSR_VL);
    old_vtype_snapshot = CSRR(CSR_VTYPE);
    asm volatile(
        "li t0, 4\n\t"
        "vsetvli t0, t0, e64, m1, ta, ma\n\t"
        ::: "t0", "memory");
    final_vtype_snapshot = CSRR(CSR_VTYPE);
    const bool final_vector_config_live =
        old_vl_snapshot == 2ULL &&
        old_vtype_snapshot != final_vtype_snapshot &&
        CSRR(CSR_VL) == final_vl &&
        CSRR(CSR_VSTART) == 0ULL;

    ai_ibc_prepare_vmodule_meip_pending(false, 0ULL);
    const bool mei_pending_but_masked =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT) &&
        (CSRR(CSR_MIE) & MIE_MEIE) == 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_ibc_ebreak_probe();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    marker += 1ULL;
    const bool breakpoint_between_vsetvl_and_vector =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe &&
        marker == 1ULL &&
        CSRR(CSR_VL) == final_vl &&
        CSRR(CSR_VTYPE) == final_vtype_snapshot &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mei_preserves_final_vector_config =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        CSRR(CSR_VL) == final_vl &&
        CSRR(CSR_VTYPE) == final_vtype_snapshot &&
        CSRR(CSR_VSTART) == 0ULL &&
        marker == 1ULL;

    ai_ibc_arm_mcontrol6_slot(0, elem2_addr, AI_IBC_MCONTROL6_STORE,
        false);
    const uint64_t trap_count_before_vector = ai_ibc_trap_count;
    ai_ibc_vector_store64_current_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    marker += 1ULL;
    const bool vector_trigger_uses_final_vtype =
        ai_ibc_trap_count == trap_count_before_vector + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_vector_store64_current_site) == 1ULL &&
        ai_ibc_log_count_bkp_tval_vstart(elem2_addr, 2ULL) == 1ULL &&
        CSRR(CSR_VL) == final_vl &&
        CSRR(CSR_VTYPE) == final_vtype_snapshot &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == dst2_seed &&
        ai_ibc_vec_dst[3] == dst3_seed &&
        marker == 2ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRW(CSR_VSTART, 0ULL);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_vector_store64_current_probe(ai_ibc_vec_src, ai_ibc_vec_dst);
    marker += 1ULL;
    const bool clear_trigger_replays_final_vector_once =
        ai_ibc_trap_count == trap_count_after_clear &&
        CSRR(CSR_VL) == final_vl &&
        CSRR(CSR_VTYPE) == final_vtype_snapshot &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == src2_payload &&
        ai_ibc_vec_dst[3] == src3_payload &&
        marker == 3ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_trap_or_source_replay =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        CSRR(CSR_VSTART) == 0ULL &&
        marker == 3ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_VSTART, 0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("after switching from an old vector shape to final e64/vl4, the masked VModule MEI and intervening breakpoint preserve the final vl/vtype state",
        final_vector_config_live &&
        mei_pending_but_masked &&
        breakpoint_between_vsetvl_and_vector);
    TEST_ASSERT("delivering the VModule MEI after the breakpoint clears only the interrupt source and leaves final vl/vtype/vstart intact",
        mei_preserves_final_vector_config);
    TEST_ASSERT("the later vector store-address trigger uses the final e64/vl4 shape, reporting element2 tval/vstart and preserving the suffix lanes",
        vector_trigger_uses_final_vtype);
    TEST_ASSERT("after clearing the trigger, the current-vtype vector store replays once from element0 through element3 with no late MEI or ghost breakpoint",
        clear_trigger_replays_final_vector_once &&
        no_late_trap_or_source_replay);

    TEST_END(ai_interrupt_m_vmodule_vsetvl_breakpoint_mei_final_vector_metadata_corner);
}

bool ai_interrupt_m_vmodule_vector_dirty_breakpoint_mei_no_pollution_corner()
{
    TEST_START();

    const uintptr_t load_site = (uintptr_t)ai_ibc_vector_load64_dirty_site;
    const uint64_t src0_payload = 0x7673646972747901ULL;
    const uint64_t src1_payload = 0x7673646972747911ULL;
    const uint64_t src2_payload = 0x7673646972747922ULL;
    const uint64_t src3_payload = 0x7673646972747933ULL;
    const uint64_t dst_seed = 0x7673646972736565ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t mstatus_clean = 0ULL;
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_vec_src[0] = src0_payload;
    ai_ibc_vec_src[1] = src1_payload;
    ai_ibc_vec_src[2] = src2_payload;
    ai_ibc_vec_src[3] = src3_payload;
    ai_ibc_vec_dst[0] = dst_seed;
    ai_ibc_vec_dst[1] = dst_seed;
    ai_ibc_vec_dst[2] = dst_seed;
    ai_ibc_vec_dst[3] = dst_seed;
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(1ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_VECTOR_DIRTY_LOG;

    CSRS(CSR_MSTATUS, AI_IBC_MSTATUS_VS_DIRTY);
    asm volatile(
        "li t0, 4\n\t"
        "vsetvli t0, t0, e64, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        ::: "t0", "memory");
    mstatus_clean = CSRR(CSR_MSTATUS);
    mstatus_clean &= ~AI_IBC_MSTATUS_VS_MASK;
    mstatus_clean |= AI_IBC_MSTATUS_VS_CLEAN;
    CSRW(CSR_MSTATUS, mstatus_clean);
    CSRW(CSR_VSTART, 0ULL);
    const bool clean_baseline_ready =
        (CSRR(CSR_MSTATUS) & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_CLEAN &&
        CSRR(CSR_VL) == 4ULL &&
        CSRR(CSR_VSTART) == 0ULL;

    ai_ibc_arm_mcontrol6_slot(0, load_site, AI_IBC_MCONTROL6_EXECUTE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_vector_load64_dirty_probe(ai_ibc_vec_src, ai_ibc_vec_dst,
        dst_seed);
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const uint64_t mstatus_after_blocked = CSRR(CSR_MSTATUS);
    const bool blocked_vector_has_no_result_or_dirty =
        ai_ibc_trap_count >= 1ULL &&
        ai_ibc_trap_count <= 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, load_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_vector_load64_dirty_after_site) == 0ULL &&
        (ai_ibc_vector_dirty_mstatus_before & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_CLEAN &&
        ai_ibc_vector_dirty_mstatus_after == 0ULL &&
        (ai_ibc_vector_dirty_mei_mstatus & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_CLEAN &&
        (mstatus_after_blocked & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_CLEAN &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_vec_dst[0] == dst_seed &&
        ai_ibc_vec_dst[1] == dst_seed &&
        ai_ibc_vec_dst[2] == dst_seed &&
        ai_ibc_vec_dst[3] == dst_seed &&
        marker == 1ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vector_dirty_mstatus_before = 0ULL;
    ai_ibc_vector_dirty_mstatus_after = 0ULL;
    ai_ibc_vector_dirty_mei_mstatus = 0ULL;
    asm volatile(
        "li t0, 4\n\t"
        "vsetvli t0, t0, e64, m1, ta, ma\n\t"
        "csrw vstart, zero\n\t"
        ::: "t0", "memory");
    CSRW(CSR_MSTATUS, mstatus_clean);
    CSRW(CSR_VSTART, 0ULL);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_vector_load64_dirty_probe(ai_ibc_vec_src, ai_ibc_vec_dst,
        dst_seed);
    marker += 1ULL;

    const bool clear_trigger_commits_vector_and_dirty =
        ai_ibc_trap_count == trap_count_after_clear &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (ai_ibc_vector_dirty_mstatus_before & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_CLEAN &&
        (ai_ibc_vector_dirty_mstatus_after & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_DIRTY &&
        (CSRR(CSR_MSTATUS) & AI_IBC_MSTATUS_VS_MASK) ==
            AI_IBC_MSTATUS_VS_DIRTY &&
        CSRR(CSR_VSTART) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == src2_payload &&
        ai_ibc_vec_dst[3] == src3_payload &&
        marker == 2ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_mei_or_breakpoint_pollution =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_vec_dst[0] == src0_payload &&
        ai_ibc_vec_dst[1] == src1_payload &&
        ai_ibc_vec_dst[2] == src2_payload &&
        ai_ibc_vec_dst[3] == src3_payload;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_VSTART, 0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VS clean, VModule MEI plus an execute breakpoint on the first vector instruction observes both events but leaves vector destination and VS dirty state untouched",
        clean_baseline_ready &&
        blocked_vector_has_no_result_or_dirty);
    TEST_ASSERT("after clearing the execute trigger and source, the same vector load/store commits once and changes VS from clean to dirty with the expected destination image",
        clear_trigger_commits_vector_and_dirty);
    TEST_ASSERT("after the committed vector path, no late VModule MEI or ghost breakpoint reuses the old clean-state episode",
        no_late_mei_or_breakpoint_pollution);

    TEST_END(ai_interrupt_m_vmodule_vector_dirty_breakpoint_mei_no_pollution_corner);
}

bool ai_interrupt_m_vmodule_rnmi_rvc_rvi_breakpoint_return_length_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    const bool rvi_length_contract =
        ai_ibc_rnmi_breakpoint_length_round(false, 0x41ULL,
            (uintptr_t)ai_ibc_rnmi_ebreak_marker_site,
            &marker);

    const bool rvc_length_contract =
        ai_ibc_rnmi_breakpoint_length_round(true, 0x23ULL,
            (uintptr_t)ai_ibc_rnmi_cebreak_marker_site,
            &marker);

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("after VModule NMI_31 is injected immediately before a 32-bit ebreak, mnret returns to the interrupted flow and the ordinary breakpoint handler skips exactly the RVI instruction",
        rvi_length_contract);
    TEST_ASSERT("after VModule NMI_31 is injected immediately before c.ebreak, mnret does not borrow the breakpoint skip and the ordinary handler skips exactly the compressed instruction",
        rvc_length_contract);

    TEST_END(ai_interrupt_m_vmodule_rnmi_rvc_rvi_breakpoint_return_length_corner);
}

bool ai_interrupt_m_vmodule_rnmi_masked_breakpoint_mei_no_half_record_corner()
{
    TEST_START();

    const uintptr_t rnmi_sentinel_epc =
        (uintptr_t)ai_interrupt_m_vmodule_rnmi_masked_breakpoint_mei_no_half_record_corner;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    const uint64_t baseline_mncause = CSRR(CSR_MNCAUSE);
    const uint64_t saved_mnepc = CSRR(CSR_MNEPC);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MNEPC, rnmi_sentinel_epc);
    CSRC(CSR_MNSTATUS, MNSTATUS_NMIE);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_masked_rnmi_ordinary_snapshot_handler);
    rnmi_handler_enabled = false;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_ebreak_probe();

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    const bool masked_window_ordinary_records =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        ai_ibc_trap_mncause[0] == baseline_mncause &&
        ai_ibc_trap_mnepc[0] == rnmi_sentinel_epc &&
        ai_ibc_trap_mncause[1] == baseline_mncause &&
        ai_ibc_trap_mnepc[1] == rnmi_sentinel_epc &&
        (ai_ibc_trap_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        (ai_ibc_trap_mnstatus[1] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_hits == 0ULL;
    const bool ordinary_interrupt_once_and_cleared =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool rnmi_csr_still_sentinel_before_enable =
        CSRR(CSR_MNCAUSE) == baseline_mncause &&
        CSRR(CSR_MNEPC) == rnmi_sentinel_epc;

    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_rnmi_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool retained_rnmi_after_enable =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != rnmi_sentinel_epc &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool rnmi_source_clear_no_repeat =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNEPC, saved_mnepc);
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("while mnstatus.NMIE is cleared, ordinary breakpoint and MEI handlers see only the preseeded RNMI CSR sentinel and never a half-updated NMI31 record",
        masked_window_ordinary_records &&
        rnmi_csr_still_sentinel_before_enable);
    TEST_ASSERT("the ordinary breakpoint and VModule MEI episodes remain exactly-once and the MEI source is cleared without being confused with the masked RNMI source",
        ordinary_interrupt_once_and_cleared);
    TEST_ASSERT("after re-enabling NMIE, the retained VModule NMI31 is delivered once through mncause/mnepc and the force source is cleared without ghost reentry",
        retained_rnmi_after_enable &&
        rnmi_source_clear_no_repeat);

    TEST_END(ai_interrupt_m_vmodule_rnmi_masked_breakpoint_mei_no_half_record_corner);
}

bool ai_interrupt_m_vmodule_rnmi_mnscratch_mscratch_log_isolation_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    const uint64_t saved_mscratch = CSRR(CSR_MSCRATCH);
    const uint64_t saved_mnscratch = CSRR(CSR_MNSCRATCH);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MSCRATCH, AI_IBC_SCRATCH_INITIAL_MS);
    CSRW(CSR_MNSCRATCH, AI_IBC_SCRATCH_INITIAL_MN);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_scratch_isolation_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_ebreak_probe();
    const bool breakpoint_rnmi_scratch_isolated =
        ai_ibc_scratch_bp_hits == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_scratch_bp_mscratch_before == AI_IBC_SCRATCH_INITIAL_MS &&
        ai_ibc_scratch_rnmi_mscratch_seen == AI_IBC_SCRATCH_BP_MS &&
        ai_ibc_scratch_rnmi_mnscratch_before ==
            AI_IBC_SCRATCH_INITIAL_MN &&
        ai_ibc_scratch_rnmi_mnscratch_written ==
            AI_IBC_SCRATCH_RNMI_MN &&
        ai_ibc_scratch_bp_mscratch_after_rnmi == AI_IBC_SCRATCH_BP_MS &&
        ai_ibc_scratch_bp_mnscratch_after_rnmi ==
            AI_IBC_SCRATCH_RNMI_MN &&
        CSRR(CSR_MSCRATCH) == AI_IBC_SCRATCH_BP_MS &&
        CSRR(CSR_MNSCRATCH) == AI_IBC_SCRATCH_RNMI_MN;

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_scratch_mei_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    const bool mei_scratch_reuses_mscratch_only =
        ai_ibc_scratch_mei_hits == 1ULL &&
        ai_ibc_scratch_mei_mscratch_before == AI_IBC_SCRATCH_BP_MS &&
        ai_ibc_scratch_mei_mnscratch_seen == AI_IBC_SCRATCH_RNMI_MN &&
        CSRR(CSR_MSCRATCH) == AI_IBC_SCRATCH_MEI_MS &&
        CSRR(CSR_MNSCRATCH) == AI_IBC_SCRATCH_RNMI_MN &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool event_log_exactly_once =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_scratch_unexpected_hits == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool cleared_sources_no_reentry =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_scratch_bp_hits == 1ULL &&
        ai_ibc_scratch_mei_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MSCRATCH, saved_mscratch);
    CSRW(CSR_MNSCRATCH, saved_mnscratch);
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when the breakpoint handler raises VModule NMI_31, the RNMI handler writes only mnscratch while preserving the ordinary mscratch value owned by the breakpoint handler",
        breakpoint_rnmi_scratch_isolated);
    TEST_ASSERT("the later VModule MEI handler reuses ordinary mscratch without overwriting the RNMI-owned mnscratch value",
        mei_scratch_reuses_mscratch_only);
    TEST_ASSERT("breakpoint, RNMI, and MEI logs each appear exactly once and clearing VModule sources prevents ghost scratch rewrites",
        event_log_exactly_once &&
        cleared_sources_no_reentry);

    TEST_END(ai_interrupt_m_vmodule_rnmi_mnscratch_mscratch_log_isolation_corner);
}

bool ai_interrupt_m_vmodule_rnmi_breakpoint_mei_source_clear_order_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_source_clear_order_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_prepare_vmodule_meip_pending(false, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_before_breakpoint = CSRR(CSR_MIP);

    ai_ibc_ebreak_probe();
    const uint64_t mip_after_breakpoint_rnmi = CSRR(CSR_MIP);

    const bool breakpoint_keeps_mei_pending_and_raises_rnmi =
        (mip_before_breakpoint & MIP_MEIP) != 0ULL &&
        ai_ibc_source_order_bp_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_ebreak_probe) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        (ai_ibc_source_order_bp_mip_before_rnmi & MIP_MEIP) != 0ULL &&
        (ai_ibc_source_order_bp_mip_after_rnmi & MIP_MEIP) != 0ULL &&
        (mip_after_breakpoint_rnmi & MIP_MEIP) != 0ULL;
    const bool rnmi_does_not_clear_or_reclassify_mei =
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        ai_ibc_source_order_mei_hits == 0ULL &&
        ai_ibc_source_order_unexpected_hits == 0ULL &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_source_order_mei_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t mip_after_mei_handler = CSRR(CSR_MIP);

    const bool ordinary_mei_clears_only_vmodule_source =
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        (ai_ibc_source_order_mei_mip_before_clear & MIP_MEIP) != 0ULL &&
        (mip_after_mei_handler & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool cleared_sources_no_ghost_reentry =
        ai_ibc_source_order_bp_hits == 1ULL &&
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI pending but ordinary interrupts disabled, the breakpoint handler can raise VModule NMI_31 and return with the MEI source still pending",
        breakpoint_keeps_mei_pending_and_raises_rnmi);
    TEST_ASSERT("the RNMI handler records NMI_31 only in RNMI CSR state and never clears or reclassifies the pending VModule MEI source",
        rnmi_does_not_clear_or_reclassify_mei);
    TEST_ASSERT("after ordinary interrupts are re-enabled, exactly one MEI handler clears vm_reg_0 and the cleared VModule sources do not cause ghost reentry",
        ordinary_mei_clears_only_vmodule_source &&
        cleared_sources_no_ghost_reentry);

    TEST_END(ai_interrupt_m_vmodule_rnmi_breakpoint_mei_source_clear_order_corner);
}

bool ai_interrupt_m_vmodule_rnmi_breakpoint_preserves_mie_control_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_source_clear_order_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mstatus_before_breakpoint = CSRR(CSR_MSTATUS);
    const uint64_t mie_before_breakpoint = CSRR(CSR_MIE);

    ai_ibc_ebreak_probe();
    const uint64_t mstatus_after_breakpoint = CSRR(CSR_MSTATUS);
    const uint64_t mie_after_breakpoint = CSRR(CSR_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rnmi_keeps_breakpoint_enable_contract =
        (mstatus_before_breakpoint & MSTATUS_MIE) == 0ULL &&
        (mie_before_breakpoint & MIE_MEIE) != 0ULL &&
        ai_ibc_source_order_bp_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_ebreak_probe) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        (ai_ibc_source_order_bp_mstatus_after_rnmi & MSTATUS_MIE) == 0ULL &&
        (ai_ibc_source_order_bp_mstatus_after_rnmi &
            AI_IBC_MSTATUS_MPIE) == 0ULL &&
        (ai_ibc_source_order_bp_mie_after_rnmi & MIE_MEIE) != 0ULL &&
        (ai_ibc_source_order_bp_mip_before_rnmi & MIP_MEIP) != 0ULL &&
        (ai_ibc_source_order_bp_mip_after_rnmi & MIP_MEIP) != 0ULL &&
        ai_ibc_source_order_mei_hits == 0ULL;
    const bool breakpoint_return_still_waits_for_explicit_mie =
        (mstatus_after_breakpoint & MSTATUS_MIE) == 0ULL &&
        (mie_after_breakpoint & MIE_MEIE) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL;

    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_source_order_mei_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t mip_after_explicit_enable = CSRR(CSR_MIP);

    const bool explicit_mie_delivers_one_mei =
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        (ai_ibc_source_order_mei_mip_before_clear & MIP_MEIP) != 0ULL &&
        (ai_ibc_source_order_mei_mip_after_clear & MIP_MEIP) == 0ULL &&
        (mip_after_explicit_enable & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool cleared_sources_do_not_reenter =
        ai_ibc_source_order_bp_hits == 1ULL &&
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_source_order_unexpected_hits == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule NMI_31 interrupts the breakpoint handler, mnret restores the breakpoint flow without opening mstatus.MIE or clearing the local MEIE enable",
        rnmi_keeps_breakpoint_enable_contract);
    TEST_ASSERT("after the breakpoint handler returns, the pending VModule MEI remains masked until the testcase explicitly sets mstatus.MIE",
        breakpoint_return_still_waits_for_explicit_mie);
    TEST_ASSERT("once mstatus.MIE is explicitly enabled, the same pending VModule MEI is delivered exactly once, clears vm_reg_0, and does not re-enter after source clear",
        explicit_mie_delivers_one_mei &&
        cleared_sources_do_not_reenter);

    TEST_END(ai_interrupt_m_vmodule_rnmi_breakpoint_preserves_mie_control_corner);
}

bool ai_interrupt_m_vmodule_mei_clear_source_rnmi_no_reassert_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_source_clear_order_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_RNMI_DURING_MEI_CLEAR;

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_source_order_mei_hits >= 1ULL &&
            ai_ibc_rnmi_hits >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t mip_after_interleaved_clear = CSRR(CSR_MIP);
    const uint64_t trap_count_after_interleaved_clear =
        ai_ibc_trap_count;
    const bool mei_clear_survives_rnmi_interleave =
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_MEI &&
        (ai_ibc_source_order_mei_mip_before_clear & MIP_MEIP) != 0ULL &&
        (ai_ibc_source_order_mei_mip_after_clear & MIP_MEIP) == 0ULL &&
        (mip_after_interleaved_clear & MIP_MEIP) == 0ULL;
    const bool rnmi_record_does_not_overwrite_mei_record =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_trap_epc[0] != ai_ibc_rnmi_mnepc[0] &&
        ai_ibc_source_order_unexpected_hits == 0ULL;

    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    const bool cleared_level_source_does_not_reassert =
        ai_ibc_source_order_mei_hits == 1ULL &&
        ai_ibc_trap_count == trap_count_after_interleaved_clear &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule MEI handler clears vm_reg_0 and VModule NMI_31 arrives in that clear-source window, the MEIP level source is still driven low before mret",
        mei_clear_survives_rnmi_interleave);
    TEST_ASSERT("the interleaved RNMI record stays in mncause/mnepc while the ordinary MEI record remains the sole mcause/mepc log entry",
        rnmi_record_does_not_overwrite_mei_record);
    TEST_ASSERT("after returning from the interleaved clear-source episode and re-enabling MEIE/MIE, the level-sensitive VModule MEI source does not reassert or re-enter",
        cleared_level_source_does_not_reassert);

    TEST_END(ai_interrupt_m_vmodule_mei_clear_source_rnmi_no_reassert_corner);
}

bool ai_interrupt_m_vmodule_rnmi_mei_breakpoint_winner_unique_replay_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t guard_seed = 0x726e6d69776e6701ULL;
    const uint64_t target_seed = 0x726e6d69776e7402ULL;
    const uint64_t after_seed = 0x726e6d69776e6103ULL;
    const uint64_t first_blocked_payload = 0x726e6d69776e6210ULL;
    const uint64_t plain_payload = 0x726e6d69776e7020ULL;
    const uint64_t replay_blocked_payload = 0x726e6d69776e6230ULL;
    const uint64_t final_payload = 0x726e6d69776e6640ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, first_blocked_payload);

    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mixed_event_set_unique =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 1ULL &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool first_breakpoint_side_effect_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_store_once(target_addr, plain_payload);
    const bool cleared_sources_plain_store_no_ghost =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == plain_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    ai_ibc_store_once(target_addr, replay_blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool replay_breakpoint_visible_once =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 2ULL &&
        ai_ibc_rnmi_mret_markers == 3ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == plain_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_store_once(target_addr, final_payload);
    const bool final_clear_no_late_repeat =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule NMI_31, VModule MEI, and an execute breakpoint armed together, the black-box handler log contains each trap source exactly once without using fixed-order assumptions",
        mixed_event_set_unique &&
        first_breakpoint_side_effect_suppressed);
    TEST_ASSERT("after clearing RNMI/MEI sources and the trigger, a plain store commits without ghost RNMI, MEI, or breakpoint replay",
        cleared_sources_plain_store_no_ghost);
    TEST_ASSERT("rearming only the execute breakpoint after the mixed redirect episode makes the breakpoint observable exactly once again, then final clear commits normally",
        replay_breakpoint_visible_once &&
        final_clear_no_late_repeat);

    TEST_END(ai_interrupt_m_vmodule_rnmi_mei_breakpoint_winner_unique_replay_corner);
}

bool ai_interrupt_m_wfi_stip_wakeup_then_ebreak_precise_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t marker = 0ULL;

    ai_ibc_prepare_stip_pending(true);

    TEST_SETUP_EXCEPT();
    ai_ibc_wfi_then_ebreak_probe(&marker);

    const bool breakpoint_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_wfi_ebreak_target;
    const bool marker_once = marker == 0x776669ULL;
    const bool pending_preserved_after_breakpoint =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("WFI returns with local STIE and pending STIP while global MIE remains closed, then reaches the precise ebreak target",
        breakpoint_seen);
    TEST_ASSERT("after the WFI-adjacent breakpoint skip, the post-ebreak marker runs once and STIP remains pending",
        marker_once &&
        pending_preserved_after_breakpoint);
    TEST_ASSERT("only after the testcase explicitly opens global MIE is the preserved STIP delivered and cleared",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_wfi_stip_wakeup_then_ebreak_precise_corner);
}

bool ai_interrupt_m_vmodule_wfi_execute_trigger_wakeup_exactly_once_corner()
{
    TEST_START();

    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t target_seed = 0x7766697472677401ULL;
    const uint64_t guard_seed = 0x7766697472676702ULL;
    const uint64_t after_seed = 0x7766697472676103ULL;
    const uint64_t blocked_payload0 = 0x776669747267b010ULL;
    const uint64_t blocked_payload1 = 0x776669747267b020ULL;
    const uint64_t final_payload = 0x776669747267f030ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(target_addr, target_seed);
    write64(guard_addr, guard_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_wfi_store_site,
        AI_IBC_MCONTROL6_EXECUTE, false);

    ai_ibc_prepare_vmodule_meip_pending(true, 32ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, blocked_payload0,
        &marker, 0x11ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round0_wfi_interrupt_then_breakpoint =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_MEI &&
        (ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_wfi_then_store_probe ||
            ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_wfi_store_site) &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_wfi_store_site &&
        marker == 0x11ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool round0_store_suppressed =
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_prepare_vmodule_meip_pending(true, 32ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, blocked_payload1,
        &marker, 0x22ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round1_replay_has_fresh_pair =
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_trap_cause[2] == CAUSE_MEI &&
        (ai_ibc_trap_epc[2] == (uintptr_t)ai_ibc_wfi_then_store_probe ||
            ai_ibc_trap_epc[2] == (uintptr_t)ai_ibc_wfi_store_site) &&
        ai_ibc_trap_cause[3] == CAUSE_BKP &&
        ai_ibc_trap_epc[3] == (uintptr_t)ai_ibc_wfi_store_site &&
        marker == 0x33ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool round1_no_duplicate_commit =
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_prepare_vmodule_meip_pending(true, 32ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, final_payload,
        &marker, 0x44ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) < 3ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool clear_trigger_allows_one_wfi_store_commit =
        ai_ibc_trap_count == 5ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 3ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        marker == 0x77ULL &&
        read64(target_addr) == final_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_wfi_or_breakpoint_replay =
        ai_ibc_trap_count == 5ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 3ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        marker == 0x77ULL &&
        read64(target_addr) == final_payload &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("round0 uses VModule MEI to wake WFI, then the first post-WFI execute-trigger store is stopped exactly once",
        round0_wfi_interrupt_then_breakpoint &&
        round0_store_suppressed);
    TEST_ASSERT("round1 repeats the WFI wakeup and first-instruction execute breakpoint with a fresh marker delta and no duplicate store commit",
        round1_replay_has_fresh_pair &&
        round1_no_duplicate_commit);
    TEST_ASSERT("after clearing the trigger, the same WFI helper commits the store once, and cleared VModule source/trigger state stays quiet",
        clear_trigger_allows_one_wfi_store_commit &&
        no_late_wfi_or_breakpoint_replay);

    TEST_END(ai_interrupt_m_vmodule_wfi_execute_trigger_wakeup_exactly_once_corner);
}

bool ai_interrupt_m_vmodule_rnmi_wfi_breakpoint_first_commit_corner()
{
    TEST_START();

    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_wfi_store_site;
    const uint64_t target_seed = 0x726e6d6977667401ULL;
    const uint64_t guard_seed = 0x726e6d6977666702ULL;
    const uint64_t after_seed = 0x726e6d6977666103ULL;
    const uint64_t blocked_payload0 = 0x726e6d697766b010ULL;
    const uint64_t blocked_payload1 = 0x726e6d697766b020ULL;
    const uint64_t final_payload = 0x726e6d697766f030ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(target_addr, target_seed);
    write64(guard_addr, guard_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);

    ai_ibc_vmodule_set_delay(16ULL);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, blocked_payload0,
        &marker, 0x31ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round0_rnmi_then_first_breakpoint =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        marker == 0x31ULL &&
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_set_delay(16ULL);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, blocked_payload1,
        &marker, 0x42ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 2ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round1_no_duplicate_or_missing_first_commit =
        ai_ibc_rnmi_hits == 2ULL &&
        ai_ibc_rnmi_mncause[1] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnret_markers == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 2ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        marker == 0x73ULL &&
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_set_delay(16ULL);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_wfi_then_store_probe((uint64_t *)target_addr, final_payload,
        &marker, 0x54ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_rnmi_hits < 3ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool clear_trigger_allows_one_post_wfi_commit =
        ai_ibc_rnmi_hits == 3ULL &&
        ai_ibc_rnmi_mnret_markers == 3ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        marker == 0xc7ULL &&
        read64(target_addr) == final_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_rnmi_or_breakpoint_replay =
        ai_ibc_rnmi_hits == 3ULL &&
        ai_ibc_rnmi_mnret_markers == 3ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        marker == 0xc7ULL &&
        read64(target_addr) == final_payload;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("round0 uses delayed VModule NMI_31 to leave WFI through RNMI and then stop the first post-WFI store with an execute breakpoint exactly once",
        round0_rnmi_then_first_breakpoint);
    TEST_ASSERT("round1 repeats the RNMI wakeup and first-instruction breakpoint with a fresh marker delta, without duplicate store commit or missing breakpoint",
        round1_no_duplicate_or_missing_first_commit);
    TEST_ASSERT("after clearing the execute trigger, delayed RNMI wakeup returns to the same WFI helper and the first post-WFI store commits once with no late RNMI/BP replay",
        clear_trigger_allows_one_post_wfi_commit &&
        no_late_rnmi_or_breakpoint_replay);

    TEST_END(ai_interrupt_m_vmodule_rnmi_wfi_breakpoint_first_commit_corner);
}

bool ai_interrupt_m_breakpoint_handler_opens_stip_rvi_rvc_pc_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_ENABLE_STIP_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_ebreak_probe();
    marker += 0x11ULL;
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rvi_pc_and_interrupt =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        marker == 0x11ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_ENABLE_STIP_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_cebreak_probe();
    marker += 0x22ULL;
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rvc_pc_and_interrupt =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_cebreak_probe &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        marker == 0x33ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when the breakpoint handler opens STIP before returning, a 32-bit ebreak still skips exactly one RVI instruction and then takes the interrupt once",
        rvi_pc_and_interrupt);
    TEST_ASSERT("when the breakpoint handler opens STIP before returning, c.ebreak still skips exactly one compressed instruction and then takes the interrupt once",
        rvc_pc_and_interrupt);

    TEST_END(ai_interrupt_m_breakpoint_handler_opens_stip_rvi_rvc_pc_corner);
}

bool ai_interrupt_m_breakpoint_handler_reads_pending_stip_preserves_delivery_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_ebreak_probe();
    marker = 0x70756e64ULL;

    const bool breakpoint_saw_pending =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        marker == 0x70756e64ULL;
    const bool pending_still_visible =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    const bool handler_did_not_enable_or_clear =
        (CSRR(CSR_MIE) & AI_IBC_MIE_STIE) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the breakpoint handler can snapshot mip.STIP while logging CAUSE_BKP without clearing the pending interrupt",
        breakpoint_saw_pending &&
        pending_still_visible);
    TEST_ASSERT("after the breakpoint handler's pending CSR read, local/global interrupt enable bits remain closed and STIP remains pending for the testcase-controlled delivery path",
        handler_did_not_enable_or_clear);

    TEST_END(ai_interrupt_m_breakpoint_handler_reads_pending_stip_preserves_delivery_corner);
}

bool ai_interrupt_m_stip_self_modifying_code_fence_i_execute_trigger_new_image_corner()
{
    TEST_START();

    typedef uint64_t (*ai_ibc_smc_func_t)(void);

    const uintptr_t code_addr = (uintptr_t)&ai_ibc_smc_code[0];
    const uint32_t old_value_inst = 0x01100513U;
    const uint32_t new_value_inst = 0x02a00513U;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    ai_ibc_smc_func_t smc_func = (ai_ibc_smc_func_t)code_addr;

    goto_priv(PRIV_M);
    ai_ibc_write_smc_code(old_value_inst);
    ai_ibc_fence_i();
    TEST_SETUP_EXCEPT();
    const uint64_t warm_value = smc_func();
    const bool old_image_warms_without_exception =
        excpt.triggered == false &&
        warm_value == 0x11ULL &&
        ai_ibc_smc_code_matches(old_value_inst);

    ai_ibc_prepare_stip_pending(false);
    ai_ibc_write_smc_code(new_value_inst);
    const uint64_t pending_before_fence = CSRR(CSR_MIP);
    ai_ibc_fence_i();
    const bool patch_and_fence_preserve_pending =
        ai_ibc_smc_code_matches(new_value_inst) &&
        (pending_before_fence & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, code_addr, AI_IBC_MCONTROL6_EXECUTE,
        false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t trap_count_before_breakpoint = ai_ibc_trap_count;
    const uint64_t breakpoint_result = smc_func();
    const uint64_t pending_after_breakpoint = CSRR(CSR_MIP);
    const bool execute_trigger_hits_new_entry_then_runs_new_body =
        breakpoint_result == 0x2aULL &&
        ai_ibc_trap_count == trap_count_before_breakpoint + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, code_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (pending_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        ai_ibc_smc_code_matches(new_value_inst);

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_after_fence_delivers_once =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        ai_ibc_smc_code_matches(new_value_inst);

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    TEST_SETUP_EXCEPT();
    const uint64_t final_value = smc_func();
    const bool clear_trigger_runs_final_new_image =
        excpt.triggered == false &&
        ai_ibc_trap_count == trap_count_after_clear &&
        final_value == 0x2aULL &&
        ai_ibc_smc_code_matches(new_value_inst) &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the executable code buffer first runs the old image, then data-side patching plus fence.i installs the new return image while STIP remains pending",
        old_image_warms_without_exception &&
        patch_and_fence_preserve_pending);
    TEST_ASSERT("with STIP pending but locally disabled, an execute-PC trigger on the patched code entry logs CAUSE_BKP at the new entry before returning through the new image",
        execute_trigger_hits_new_entry_then_runs_new_body);
    TEST_ASSERT("opening STIE/MIE after the execute breakpoint delivers the preserved STIP exactly once without corrupting the patched code bytes",
        interrupt_after_fence_delivers_once);
    TEST_ASSERT("after clearing the execute trigger, the same code buffer executes the final post-fence image without old-image or mixed-instruction fallout",
        clear_trigger_runs_final_new_image);

    TEST_END(ai_interrupt_m_stip_self_modifying_code_fence_i_execute_trigger_new_image_corner);
}

bool ai_interrupt_m_mideleg_ssip_switch_breakpoint_final_hs_delivery_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIDELEG, MIP_SSIP);
    CSRC(CSR_MIP, MIP_SSIP);
    CSRC(CSR_MIE, MIE_SSIE);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRS(CSR_MIP, MIP_SSIP);
    CSRS(CSR_MIDELEG, MIP_SSIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);

    const uint64_t mideleg_before_breakpoint = CSRR(CSR_MIDELEG);
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mideleg_after_breakpoint = CSRR(CSR_MIDELEG);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    CSRW(CSR_MTVEC, saved_mtvec);

    const bool breakpoint_does_not_consume_or_rollback_delegation =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mip[0] & MIP_SSIP) != 0ULL &&
        (mideleg_before_breakpoint & MIP_SSIP) != 0ULL &&
        (mideleg_after_breakpoint & MIP_SSIP) != 0ULL &&
        (mip_after_breakpoint & MIP_SSIP) != 0ULL &&
        marker == 1ULL;

    const bool delegated_interrupt_seen =
        ai_ibc_enable_delegated_ssip_and_wait_hs();
    const uint64_t sip_after_interrupt = CSRR(CSR_SIP);
    marker += 1ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRW(CSR_STVEC, saved_stvec);
    goto_priv(PRIV_M);
    const uint64_t mideleg_after_interrupt = CSRR(CSR_MIDELEG);
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);

    const bool final_delegation_delivers_to_hs_once =
        delegated_interrupt_seen &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (ai_ibc_trap_mip[1] & SIP_SSIP) != 0ULL &&
        (mideleg_after_interrupt & MIP_SSIP) != 0ULL &&
        (sip_after_interrupt & SIP_SSIP) == 0ULL &&
        (mip_after_interrupt & MIP_SSIP) == 0ULL &&
        marker == 2ULL;

    const bool testcase_final_delegation_survives_until_restore =
        (CSRR(CSR_MIDELEG) & MIP_SSIP) != 0ULL &&
        (CSRR(CSR_MIP) & MIP_SSIP) == 0ULL;

    CSRC(CSR_MIP, MIP_SSIP);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("after SSIP is made pending and mideleg.SSI is switched to the final delegated value, the M-mode ebreak logs CAUSE_BKP without consuming pending SSIP or rolling back delegation",
        breakpoint_does_not_consume_or_rollback_delegation,
        "count=%lx cause0=%lx epc0=%lx mip0=%lx mideleg_before=%lx mideleg_after=%lx mip_after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_mip[0], mideleg_before_breakpoint,
        mideleg_after_breakpoint, mip_after_breakpoint, marker);
    TEST_ASSERT("when the testcase later enters HS/S and opens SSIE/SIE, the preserved SSIP is delivered by the HS/S handler exactly once rather than by the M breakpoint handler",
        final_delegation_delivers_to_hs_once,
        "trap_count=%lx cause0=%lx cause1=%lx sip1=%lx mideleg_after_irq=%lx sip_after_irq=%lx mip_after_irq=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_mip[1], mideleg_after_interrupt, sip_after_interrupt,
        mip_after_interrupt, marker);
    TEST_ASSERT("the test-controlled final mideleg.SSI value remains set after the delegated interrupt is cleared, and only the restore path returns the saved delegation state",
        testcase_final_delegation_survives_until_restore);

    TEST_END(ai_interrupt_m_mideleg_ssip_switch_breakpoint_final_hs_delivery_corner);
}

bool ai_interrupt_m_mideleg_ssip_pending_address_trigger_priv_isolation_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t guard_seed = 0x4453534950475201ULL;
    const uint64_t target_seed = 0x4453534950544702ULL;
    const uint64_t after_seed = 0x4453534950414603ULL;
    const uint64_t blocked_payload = 0x4453534950424b04ULL;
    const uint64_t committed_payload = 0x4453534950434d05ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, MIE_SSIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRS(CSR_MIP, MIP_SSIP);
    CSRS(CSR_MIDELEG, MIP_SSIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t setup_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t setup_mip = CSRR(CSR_MIP);
    ai_ibc_store_once(target_addr, blocked_payload);
    marker += 1ULL;
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const uint64_t mideleg_after_breakpoint = CSRR(CSR_MIDELEG);
    uint64_t tdata1_after_breakpoint = 0ULL;
    uint64_t tdata2_after_breakpoint = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_breakpoint,
        &tdata2_after_breakpoint);

    const bool breakpoint_privilege_isolated =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == store_pc &&
        ai_ibc_trap_magic[0] == AI_IBC_NESTED_LOG_MAGIC_BASE &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (setup_mideleg & MIP_SSIP) != 0ULL &&
        (setup_mideleg & AI_IBC_MIP_STIP) == 0ULL &&
        (setup_mip & MIP_SSIP) != 0ULL &&
        (setup_mip & AI_IBC_MIP_STIP) == 0ULL &&
        (ai_ibc_trap_mip[0] & MIP_SSIP) != 0ULL &&
        (ai_ibc_trap_tval[0] == target_addr) &&
        (mip_after_breakpoint & MIP_SSIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        marker == 1ULL;

    const bool breakpoint_does_not_consume_delegation_or_trigger =
        (mideleg_after_breakpoint & MIP_SSIP) != 0ULL &&
        (mideleg_after_breakpoint & AI_IBC_MIP_STIP) == 0ULL &&
        ai_ibc_mcontrol6_type(tdata1_after_breakpoint) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        tdata2_after_breakpoint == target_addr;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool delegated_interrupt_seen =
        ai_ibc_enable_delegated_ssip_and_wait_hs();
    const uint64_t sip_after_interrupt = CSRR(CSR_SIP);
    marker += 1ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRW(CSR_STVEC, saved_stvec);
    goto_priv(PRIV_M);
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);
    const uint64_t mideleg_after_interrupt = CSRR(CSR_MIDELEG);
    const bool delegated_ssip_delivered_once =
        delegated_interrupt_seen &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_trap_magic[1] == (AI_IBC_HS_LOG_MAGIC_BASE + 1ULL) &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (ai_ibc_trap_mip[1] & SIP_SSIP) != 0ULL &&
        (sip_after_interrupt & SIP_SSIP) == 0ULL &&
        (sip_after_interrupt & SIP_STIP) == 0ULL &&
        (mip_after_interrupt & MIP_SSIP) == 0ULL &&
        (mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        (mideleg_after_interrupt & MIP_SSIP) != 0ULL &&
        (mideleg_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 2ULL;

    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, committed_payload);
    const bool unrelated_pending_and_side_effect_contract =
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (CSRR(CSR_MIP) & (MIP_SSIP | AI_IBC_MIP_STIP)) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("with delegated SSIP pending but SSIE/SIE still closed, the M-mode address trigger logs a single M breakpoint and suppresses only the trapped store",
        breakpoint_privilege_isolated,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx magic0=%lx mstatus0=%lx mip0=%lx mip_after=%lx target=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_tval[0], ai_ibc_trap_magic[0],
        ai_ibc_trap_mstatus[0], ai_ibc_trap_mip[0],
        mip_after_breakpoint, read64(target_addr), marker);
    TEST_ASSERT("the M breakpoint handler does not consume mideleg.SSI, clear the delegated SSIP, or rewrite the address trigger configuration",
        breakpoint_does_not_consume_delegation_or_trigger,
        "mideleg_after=%lx tdata1=%lx tdata2=%lx target_addr=%lx",
        mideleg_after_breakpoint, tdata1_after_breakpoint,
        tdata2_after_breakpoint, target_addr);
    TEST_ASSERT("after switching to HS/S and opening only SSIE/SIE, the delegated SSIP handler runs exactly once, clears only SSIP, and the cleared trigger then lets one store commit",
        delegated_ssip_delivered_once &&
        unrelated_pending_and_side_effect_contract,
        "count=%lx cause0=%lx cause1=%lx magic1=%lx sip_after=%lx mip_after_irq=%lx mideleg_after_irq=%lx target=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_magic[1], sip_after_interrupt, mip_after_interrupt,
        mideleg_after_interrupt, read64(target_addr), marker);

    TEST_END(ai_interrupt_m_mideleg_ssip_pending_address_trigger_priv_isolation_corner);
}

bool ai_interrupt_m_mideleg_ssip_m_only_trigger_privilege_no_cross_fire_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t guard_seed = 0x4d53534950524701ULL;
    const uint64_t target_seed = 0x4d53534950525402ULL;
    const uint64_t after_seed = 0x4d53534950524103ULL;
    const uint64_t hs_payload = 0x4d53534950524804ULL;
    const uint64_t m_blocked_payload = 0x4d53534950524d05ULL;
    const uint64_t final_payload = 0x4d53534950524606ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    volatile uint64_t marker = 0ULL;
    uint64_t armed_tdata1 = 0ULL;
    uint64_t armed_tdata2 = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, MIE_SSIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRS(CSR_MIP, MIP_SSIP);
    CSRS(CSR_MIDELEG, MIP_SSIP);
    ai_ibc_arm_mcontrol6_slot_priv(0, target_addr,
        AI_IBC_MCONTROL6_STORE, AI_IBC_MCONTROL6_M, false);
    ai_ibc_read_trigger_csr_slot(0, &armed_tdata1, &armed_tdata2);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, hs_payload);
    marker += 1ULL;
    const uint64_t hs_sip_after_store = CSRR(CSR_SIP);
    const uint64_t hs_trap_count_after_store = ai_ibc_trap_count;
    const bool hs_priv_mismatch_does_not_fire =
        excpt.triggered == false &&
        hs_trap_count_after_store == 0ULL &&
        (hs_sip_after_store & SIP_SSIP) != 0ULL &&
        (CSRR(CSR_SSTATUS) & SSTATUS_SIE_BIT) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == hs_payload &&
        read64(after_addr) == after_seed &&
        marker == 1ULL;

    CSRW(CSR_STVEC, saved_stvec);
    goto_priv(PRIV_M);
    const uint64_t m_mip_after_hs_store = CSRR(CSR_MIP);
    uint64_t tdata1_after_hs = 0ULL;
    uint64_t tdata2_after_hs = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_hs, &tdata2_after_hs);
    const bool delegated_ssip_and_trigger_survive_hs_mismatch =
        (m_mip_after_hs_store & MIP_SSIP) != 0ULL &&
        (m_mip_after_hs_store & AI_IBC_MIP_STIP) == 0ULL &&
        ai_ibc_mcontrol6_type(armed_tdata1) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (armed_tdata1 & AI_IBC_MCONTROL6_M) != 0ULL &&
        (armed_tdata1 & (AI_IBC_MCONTROL6_S | AI_IBC_MCONTROL6_U)) ==
            0ULL &&
        (armed_tdata1 & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        armed_tdata2 == target_addr &&
        tdata1_after_hs == armed_tdata1 &&
        tdata2_after_hs == target_addr;

    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, m_blocked_payload);
    marker += 1ULL;
    const uint64_t m_mip_after_breakpoint = CSRR(CSR_MIP);
    const bool m_priv_match_breaks_and_suppresses_store =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == store_pc &&
        ai_ibc_trap_tval[0] == target_addr &&
        ai_ibc_trap_magic[0] == AI_IBC_NESTED_LOG_MAGIC_BASE &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (ai_ibc_trap_mip[0] & MIP_SSIP) != 0ULL &&
        (m_mip_after_breakpoint & MIP_SSIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == hs_payload &&
        read64(after_addr) == after_seed &&
        marker == 2ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    const bool delegated_interrupt_seen =
        ai_ibc_enable_delegated_ssip_and_wait_hs();
    const uint64_t hs_sip_after_interrupt = CSRR(CSR_SIP);
    marker += 1ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRW(CSR_STVEC, saved_stvec);
    goto_priv(PRIV_M);
    const uint64_t m_mip_after_interrupt = CSRR(CSR_MIP);
    const bool delegated_ssip_delivers_after_m_breakpoint =
        delegated_interrupt_seen &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_trap_magic[1] == (AI_IBC_HS_LOG_MAGIC_BASE + 1ULL) &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (ai_ibc_trap_mip[1] & SIP_SSIP) != 0ULL &&
        (hs_sip_after_interrupt & SIP_SSIP) == 0ULL &&
        (m_mip_after_interrupt & MIP_SSIP) == 0ULL &&
        (m_mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 3ULL;

    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, final_payload);
    const bool cleared_trigger_allows_final_store =
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_payload &&
        read64(after_addr) == after_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("with slot0 armed as an M-only store-address trigger and delegated SSIP pending, the same HS/S store does not cross-fire and commits normally",
        hs_priv_mismatch_does_not_fire,
        "excpt=%d trap_count=%lx sip=%lx target=%lx marker=%lx",
        excpt.triggered, hs_trap_count_after_store, hs_sip_after_store,
        read64(target_addr), marker);
    TEST_ASSERT("the HS/S privilege-mismatch pass preserves both mideleg.SSI pending state and the exact M-only trigger CSR tuple",
        delegated_ssip_and_trigger_survive_hs_mismatch,
        "mip=%lx armed_tdata1=%lx armed_tdata2=%lx after_tdata1=%lx after_tdata2=%lx",
        m_mip_after_hs_store, armed_tdata1, armed_tdata2,
        tdata1_after_hs, tdata2_after_hs);
    TEST_ASSERT("after returning to M-mode, the same M-only trigger reports one breakpoint, suppresses the M store, and still does not consume delegated SSIP",
        m_priv_match_breaks_and_suppresses_store,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx mip0=%lx mip_after=%lx target=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_tval[0], ai_ibc_trap_mip[0],
        m_mip_after_breakpoint, read64(target_addr), marker);
    TEST_ASSERT("once the trigger is cleared and HS/S enables SSIE/SIE, the delegated SSIP handler runs exactly once and the final store commits without ghost breakpoint",
        delegated_ssip_delivers_after_m_breakpoint &&
        cleared_trigger_allows_final_store,
        "count=%lx cause0=%lx cause1=%lx magic1=%lx sip_after=%lx mip_after=%lx target=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_magic[1], hs_sip_after_interrupt,
        m_mip_after_interrupt, read64(target_addr), marker);

    TEST_END(ai_interrupt_m_mideleg_ssip_m_only_trigger_privilege_no_cross_fire_corner);
}

bool ai_interrupt_hs_sum_mxr_breakpoint_delegated_ssip_permission_window_corner()
{
    TEST_START();

    const uintptr_t sum_va = hs_page_base(VSURWX_GRWX);
    const uintptr_t sum_pa = phys_page_base(VSURWX_GRWX);
    const uintptr_t mxr_va = hs_page_base(VSX_GRWX);
    const uintptr_t mxr_pa = phys_page_base(VSX_GRWX);
    const uint64_t sum_payload = 0x53554d4250495401ULL;
    const uint64_t mxr_payload = 0x4d58524250495402ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(sum_pa, sum_payload);
    write64(mxr_pa, mxr_payload);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_SSIP);
    CSRC(CSR_MIDELEG, MIP_SSIP);
    CSRS(CSR_MIDELEG, MIP_SSIP);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT | SSTATUS_SUM | SSTATUS_MXR);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    hspt_init();
    sfence_vma();

    goto_priv(PRIV_HS);

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT | SSTATUS_MXR);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRS(CSR_SIP, SIP_SSIP);
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t sum_sstatus_after_breakpoint = CSRR(CSR_SSTATUS);
    const uint64_t sum_sip_after_breakpoint = CSRR(CSR_SIP);

    const bool sum_breakpoint_preserves_permission_window =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_magic[0] == AI_IBC_HS_LOG_MAGIC_BASE &&
        (ai_ibc_trap_mstatus[0] & SSTATUS_SUM) != 0ULL &&
        (ai_ibc_trap_mstatus[0] & SSTATUS_MXR) == 0ULL &&
        (sum_sstatus_after_breakpoint & SSTATUS_SUM) != 0ULL &&
        (sum_sstatus_after_breakpoint & SSTATUS_MXR) == 0ULL &&
        (sum_sip_after_breakpoint & SIP_SSIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL &&
        marker == 1ULL;

    CSRS(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    const uint64_t sum_sstatus_after_interrupt = CSRR(CSR_SSTATUS);
    const uint64_t sum_sip_after_interrupt = CSRR(CSR_SIP);
    CSRW(CSR_STVEC, saved_stvec);
    TEST_SETUP_EXCEPT();
    const uint64_t sum_loaded = ai_ibc_load_once(sum_va);
    const bool sum_load_fault_free = !excpt.triggered;
    excpt.testing = false;
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    marker += 1ULL;

    const bool delegated_ssip_keeps_sum_and_load_permission =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_trap_magic[1] == (AI_IBC_HS_LOG_MAGIC_BASE + 1ULL) &&
        (ai_ibc_trap_mstatus[1] & SSTATUS_SUM) != 0ULL &&
        (ai_ibc_trap_mstatus[1] & SSTATUS_MXR) == 0ULL &&
        (sum_sstatus_after_interrupt & SSTATUS_SUM) != 0ULL &&
        (sum_sstatus_after_interrupt & SSTATUS_MXR) == 0ULL &&
        (sum_sip_after_interrupt & SIP_SSIP) == 0ULL &&
        sum_load_fault_free &&
        sum_loaded == sum_payload &&
        marker == 2ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT | SSTATUS_SUM);
    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRS(CSR_SIP, SIP_SSIP);
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mxr_sstatus_after_breakpoint = CSRR(CSR_SSTATUS);
    const uint64_t mxr_sip_after_breakpoint = CSRR(CSR_SIP);

    const bool mxr_breakpoint_preserves_permission_window =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[2] == CAUSE_BKP &&
        ai_ibc_trap_epc[2] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_magic[2] == (AI_IBC_HS_LOG_MAGIC_BASE + 2ULL) &&
        (ai_ibc_trap_mstatus[2] & SSTATUS_SUM) == 0ULL &&
        (ai_ibc_trap_mstatus[2] & SSTATUS_MXR) != 0ULL &&
        (mxr_sstatus_after_breakpoint & SSTATUS_SUM) == 0ULL &&
        (mxr_sstatus_after_breakpoint & SSTATUS_MXR) != 0ULL &&
        (mxr_sip_after_breakpoint & SIP_SSIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        marker == 3ULL;

    CSRS(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    const uint64_t mxr_sstatus_after_interrupt = CSRR(CSR_SSTATUS);
    const uint64_t mxr_sip_after_interrupt = CSRR(CSR_SIP);
    CSRW(CSR_STVEC, saved_stvec);
    TEST_SETUP_EXCEPT();
    const uint64_t mxr_loaded = ai_ibc_load_once(mxr_va);
    const bool mxr_load_fault_free = !excpt.triggered;
    excpt.testing = false;
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    marker += 1ULL;

    const bool delegated_ssip_keeps_mxr_and_execute_only_load =
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_trap_cause[3] == CAUSE_SSI &&
        ai_ibc_trap_magic[3] == (AI_IBC_HS_LOG_MAGIC_BASE + 3ULL) &&
        (ai_ibc_trap_mstatus[3] & SSTATUS_SUM) == 0ULL &&
        (ai_ibc_trap_mstatus[3] & SSTATUS_MXR) != 0ULL &&
        (mxr_sstatus_after_interrupt & SSTATUS_SUM) == 0ULL &&
        (mxr_sstatus_after_interrupt & SSTATUS_MXR) != 0ULL &&
        (mxr_sip_after_interrupt & SIP_SSIP) == 0ULL &&
        mxr_load_fault_free &&
        mxr_loaded == mxr_payload &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 2ULL &&
        marker == 4ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRW(CSR_STVEC, saved_stvec);
    goto_priv(PRIV_M);
    CSRC(CSR_MIP, MIP_SSIP);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("with SUM set and delegated SSIP pending but disabled, the HS/S ebreak records one breakpoint and preserves the SUM-only permission window",
        sum_breakpoint_preserves_permission_window,
        "count=%lx cause0=%lx magic0=%lx trap_sstatus0=%lx sstatus_after=%lx sip_after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_magic[0],
        ai_ibc_trap_mstatus[0], sum_sstatus_after_breakpoint,
        sum_sip_after_breakpoint, marker);
    TEST_ASSERT("after opening SSIE/SIE, delegated SSIP runs once, clears only its pending bit, and the SUM-protected U page load still succeeds",
        delegated_ssip_keeps_sum_and_load_permission,
        "cause1=%lx magic1=%lx trap_sstatus1=%lx sstatus_after=%lx sip_after=%lx loaded=%lx expected=%lx",
        ai_ibc_trap_cause[1], ai_ibc_trap_magic[1],
        ai_ibc_trap_mstatus[1], sum_sstatus_after_interrupt,
        sum_sip_after_interrupt, sum_loaded, sum_payload);
    TEST_ASSERT("with MXR set and SUM clear, the second HS/S ebreak records one breakpoint while delegated SSIP stays pending and the MXR-only permission window is preserved",
        mxr_breakpoint_preserves_permission_window,
        "count=%lx cause2=%lx magic2=%lx trap_sstatus2=%lx sstatus_after=%lx sip_after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[2], ai_ibc_trap_magic[2],
        ai_ibc_trap_mstatus[2], mxr_sstatus_after_breakpoint,
        mxr_sip_after_breakpoint, marker);
    TEST_ASSERT("after the second delegated SSIP, MXR remains set, SUM remains clear, and an execute-only page load succeeds only under the final MXR permission setting",
        delegated_ssip_keeps_mxr_and_execute_only_load,
        "cause3=%lx magic3=%lx trap_sstatus3=%lx sstatus_after=%lx sip_after=%lx loaded=%lx expected=%lx",
        ai_ibc_trap_cause[3], ai_ibc_trap_magic[3],
        ai_ibc_trap_mstatus[3], mxr_sstatus_after_interrupt,
        mxr_sip_after_interrupt, mxr_loaded, mxr_payload);

    TEST_END(ai_interrupt_hs_sum_mxr_breakpoint_delegated_ssip_permission_window_corner);
}

bool ai_interrupt_m_stip_address_trigger_clear_order_no_cross_clear_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x4164647254726701ULL;
    const uint64_t target_seed = 0x4164647254727402ULL;
    const uint64_t after_seed = 0x4164647254726103ULL;
    const uint64_t blocked_payload = 0x4164647254726204ULL;
    const uint64_t committed_payload = 0x4164647254726305ULL;
    const uint64_t second_blocked_payload = 0x4164647254726406ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_store_once(target_addr, blocked_payload);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const bool breakpoint_did_not_clear_stip =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_store_once &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool interrupt_seen =
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL;
    const bool interrupt_did_not_modify_target =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_store_once(target_addr, committed_payload);
    const bool trigger_clear_allows_commit =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool second_interrupt_seen =
        ai_ibc_log_count_cause(CAUSE_STI) == 2ULL;
    const bool interrupt_first_did_not_clear_trigger =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_store_once(target_addr, second_blocked_payload);
    const bool interrupt_first_then_breakpoint =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_trap_cause[2] == CAUSE_STI &&
        ai_ibc_trap_cause[3] == CAUSE_BKP &&
        ai_ibc_trap_epc[3] == (uintptr_t)ai_ibc_store_once &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("an address-trigger breakpoint logs mip.STIP pending but does not clear the unrelated timer source or commit the trapped store",
        breakpoint_did_not_clear_stip);
    TEST_ASSERT("after clearing only the trigger slot, the STIP handler clears only STIP and leaves the target memory image unchanged",
        interrupt_seen &&
        interrupt_did_not_modify_target);
    TEST_ASSERT("the post-interrupt trigger CSR state is testcase-controlled: clear lets a store commit, and rearm produces a fresh breakpoint",
        trigger_clear_allows_commit &&
        second_interrupt_seen &&
        interrupt_first_did_not_clear_trigger &&
        interrupt_first_then_breakpoint);

    TEST_END(ai_interrupt_m_stip_address_trigger_clear_order_no_cross_clear_corner);
}

bool ai_interrupt_m_ssip_stip_priority_after_breakpoint_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRC(CSR_MIE, MIE_SSIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_MIP, MIP_SSIP);
    CSRS(CSR_MIP, MIP_SSIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    const uint64_t pending_before_breakpoint = CSRR(CSR_MIP);
    ai_ibc_ebreak_probe();
    marker += 0x10ULL;
    const uint64_t pending_after_breakpoint = CSRR(CSR_MIP);

    const bool breakpoint_preserved_pending_set =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (pending_before_breakpoint & MIP_SSIP) != 0ULL &&
        (pending_before_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (pending_after_breakpoint & MIP_SSIP) != 0ULL &&
        (pending_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & MIP_SSIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        marker == 0x10ULL;

    CSRS(CSR_MIE, MIE_SSIE | AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL ||
         ai_ibc_log_count_cause(CAUSE_STI) == 0ULL); ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 0x20ULL;

    const bool priority_order_preserved =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_trap_cause[2] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & MIP_SSIP) != 0ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[2] & MIP_SSIP) == 0ULL &&
        (ai_ibc_trap_mip[2] & AI_IBC_MIP_STIP) != 0ULL;
    const bool pending_cleared_exactly_once =
        (CSRR(CSR_MIP) & (MIP_SSIP | AI_IBC_MIP_STIP)) == 0ULL &&
        marker == 0x30ULL;

    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with SSIP and STIP both pending but locally disabled, the breakpoint log snapshots the complete pending set without clearing or reordering it",
        breakpoint_preserved_pending_set,
        "count=%lx cause0=%lx epc0=%lx mip0=%lx pending_before=%lx pending_after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_mip[0], pending_before_breakpoint,
        pending_after_breakpoint, marker);
    TEST_ASSERT("after enabling both local interrupt bits and global MIE, the supervisor software interrupt is handled before the supervisor timer interrupt",
        priority_order_preserved,
        "count=%lx cause0=%lx cause1=%lx cause2=%lx mip1=%lx mip2=%lx counts=%lx/%lx/%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_cause[2], ai_ibc_trap_mip[1], ai_ibc_trap_mip[2],
        ai_ibc_log_count_cause(CAUSE_BKP),
        ai_ibc_log_count_cause(CAUSE_SSI),
        ai_ibc_log_count_cause(CAUSE_STI));
    TEST_ASSERT("each pending interrupt is cleared once after the priority-ordered delivery and the main marker advances exactly once past the interrupt window",
        pending_cleared_exactly_once,
        "marker=%lx mip=%lx", marker, CSRR(CSR_MIP));

    TEST_END(ai_interrupt_m_ssip_stip_priority_after_breakpoint_corner);
}

bool ai_interrupt_m_vectored_mtvec_ebreak_direct_stip_vector_slot_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, ((uintptr_t)ai_ibc_mtvec_vectored_table) | 1ULL);

    ai_ibc_ebreak_probe();
    marker += 1ULL;

    const bool breakpoint_uses_direct_entry =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_vectored_direct_hits == 1ULL &&
        ai_ibc_vectored_stip_hits == 0ULL &&
        ai_ibc_vectored_unexpected_hits == 0ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        marker == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_vectored_stip_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool interrupt_uses_vector_slot =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_vectored_direct_hits == 1ULL &&
        ai_ibc_vectored_stip_hits == 1ULL &&
        ai_ibc_vectored_unexpected_hits == 0ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 2ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with mtvec in vectored mode, synchronous ebreak still enters BASE direct entry and skips the breakpoint once",
        breakpoint_uses_direct_entry);
    TEST_ASSERT("after STIE/MIE are opened, the preserved STIP interrupt enters BASE+4*cause vector slot and clears only STIP",
        interrupt_uses_vector_slot);

    TEST_END(ai_interrupt_m_vectored_mtvec_ebreak_direct_stip_vector_slot_corner);
}

bool ai_interrupt_m_breakpoint_handler_switch_mtvec_pending_stip_new_vector_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SWITCH_MTVEC_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_ebreak_probe();
    marker += 1ULL;

    const uint64_t mtvec_after_breakpoint = CSRR(CSR_MTVEC);
    const bool breakpoint_used_old_direct_handler =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_new_vectored_stip_hits == 0ULL &&
        ai_ibc_vectored_unexpected_hits == 0ULL &&
        (mtvec_after_breakpoint ==
            (((uintptr_t)ai_ibc_mtvec_new_vectored_table) | 1ULL)) &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        marker == 1ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_new_vectored_stip_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool pending_interrupt_used_new_vector =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_new_vectored_stip_hits == 1ULL &&
        ai_ibc_vectored_stip_hits == 0ULL &&
        ai_ibc_vectored_unexpected_hits == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 2ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the breakpoint first enters the original direct mtvec handler, and that handler switches mtvec to the new vectored table",
        breakpoint_used_old_direct_handler);
    TEST_ASSERT("after STIE/MIE are opened, the preserved STIP interrupt uses the newly installed vector base rather than the old direct handler",
        pending_interrupt_used_new_vector);

    TEST_END(ai_interrupt_m_breakpoint_handler_switch_mtvec_pending_stip_new_vector_corner);
}

bool ai_interrupt_m_stip_handler_ebreak_same_source_no_reentry_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t main_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_stip_handler_ebreak_probe);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_nested_after_marker == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    main_marker = 0x6e657374ULL;

    const bool interrupt_then_breakpoint_order =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] ==
            (uintptr_t)ai_ibc_nested_handler_ebreak_site;
    const bool handler_progress_once =
        ai_ibc_nested_after_marker == 0x73746962ULL &&
        main_marker == 0x6e657374ULL;
    const bool no_same_source_reentry =
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the enabled STIP enters the custom interrupt handler before the handler-local ebreak is taken",
        interrupt_then_breakpoint_order);
    TEST_ASSERT("the breakpoint inside the interrupt handler skips only its own ebreak and returns to the handler continuation exactly once",
        handler_progress_once);
    TEST_ASSERT("while inside the handler-local breakpoint episode, trap entry keeps the same STIP source from re-entering until the handler clears it",
        no_same_source_reentry);

    TEST_END(ai_interrupt_m_stip_handler_ebreak_same_source_no_reentry_corner);
}

bool ai_interrupt_m_stip_clear_sequence_nested_breakpoint_no_reentry_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t main_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_stip_handler_ebreak_probe);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_nested_after_marker == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    main_marker = 0x636c7273ULL;

    const bool nested_during_clear_sequence =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] ==
            (uintptr_t)ai_ibc_nested_handler_ebreak_site &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL;
    const bool handler_resumed_and_cleared_once =
        ai_ibc_nested_after_marker == 0x73746962ULL &&
        main_marker == 0x636c7273ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_reentry_after_clear =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    main_marker += 0x100ULL;

    const bool later_timer_event_not_lost =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        main_marker == 0x636c7373ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the timer/STIP handler takes its local breakpoint before the pending clear, so both slots see the same pending source without re-entering it",
        nested_during_clear_sequence);
    TEST_ASSERT("after the nested breakpoint returns, the timer handler reaches the clear tail once and the original pending bit is gone",
        handler_resumed_and_cleared_once &&
        no_reentry_after_clear);
    TEST_ASSERT("a later testcase-controlled STIP can still be delivered exactly once, proving the clear sequence did not lose or mask the timer source",
        later_timer_event_not_lost);

    TEST_END(ai_interrupt_m_stip_clear_sequence_nested_breakpoint_no_reentry_corner);
}

bool ai_interrupt_m_vmodule_meip_write_protocol_interrupted_no_half_state_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRC(CSR_MIE, MIE_MEIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    ai_ibc_vmodule_issue_addr_cmd(AI_IBC_VMODULE_REG_MAIN_INTERRUPT);
    CSRS(CSR_MIP, AI_IBC_MIP_STIP);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool address_half_does_not_raise_meip =
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 1ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MIE, MIE_MEIE);
    ai_ibc_vmodule_issue_data_cmd(AI_IBC_VMODULE_MAIN_MEI);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool data_half_completes_one_mei =
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 2ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_half_write_repeat =
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("interrupting the VModule address-command half of vm_reg_0 write does not form a partial MEIP injection or leave STIP pending",
        address_half_does_not_raise_meip);
    TEST_ASSERT("after interrupt return, issuing the matching VModule data-command half produces exactly one MEI that clears through vm_reg_0",
        data_half_completes_one_mei);
    TEST_ASSERT("the interrupted VModule write protocol does not repeat from stale half-state after both sources are cleared",
        no_half_write_repeat);

    TEST_END(ai_interrupt_m_vmodule_meip_write_protocol_interrupted_no_half_state_corner);
}

bool ai_interrupt_m_vmodule_meip_storm_breakpoint_rearm_clear_once_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x766d73746f726d01ULL;
    const uint64_t target_seed = 0x766d73746f726d02ULL;
    const uint64_t after_seed = 0x766d73746f726d03ULL;
    const uint64_t blocked_payload0 = 0x766d73746f726d10ULL;
    const uint64_t control_payload0 = 0x766d73746f726d11ULL;
    const uint64_t blocked_payload1 = 0x766d73746f726d20ULL;
    const uint64_t control_payload1 = 0x766d73746f726d21ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_CLEAR_SLOT0_ON_BKP;

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, blocked_payload0);
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round0_event_set_and_clear =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) &&
        ai_ibc_breakpoint_cleared_trigger_hits == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 1ULL;
    const bool round0_side_effect_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    const uint64_t trap_count_after_round0 = ai_ibc_trap_count;
    const uint64_t bkp_count_after_round0 =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, control_payload0);
    marker += 1ULL;
    const bool round0_clear_allows_control_path =
        ai_ibc_trap_count == trap_count_after_round0 &&
        ai_ibc_log_count_cause(CAUSE_BKP) == bkp_count_after_round0 &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == control_payload0 &&
        read64(after_addr) == after_seed &&
        marker == 2ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_store_once(target_addr, blocked_payload1);
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool round1_event_set_and_rearm =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 2ULL &&
        ai_ibc_breakpoint_cleared_trigger_hits == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 3ULL;
    const bool round1_side_effect_suppressed =
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == control_payload0 &&
        read64(after_addr) == after_seed;

    const uint64_t trap_count_after_round1 = ai_ibc_trap_count;
    const uint64_t bkp_count_after_round1 =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, control_payload1);
    marker += 1ULL;
    const bool round1_clear_allows_control_path =
        ai_ibc_trap_count == trap_count_after_round1 &&
        ai_ibc_log_count_cause(CAUSE_BKP) == bkp_count_after_round1 &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == control_payload1 &&
        read64(after_addr) == after_seed &&
        marker == 4ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_storm_repeat_after_sources_clear =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_breakpoint_cleared_trigger_hits == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("round0 observes one VModule MEI and one execute-trigger breakpoint, and the breakpoint handler clears slot0 once without assuming a fixed event order",
        round0_event_set_and_clear &&
        round0_side_effect_suppressed);
    TEST_ASSERT("after round0 handler clear, the same helper control path commits without a ghost breakpoint before the testcase rearms the trigger",
        round0_clear_allows_control_path);
    TEST_ASSERT("round1 VModule MEI reinjection plus rearmed execute trigger produces one fresh pair of events and proves the previous clear did not permanently mask slot0",
        round1_event_set_and_rearm &&
        round1_side_effect_suppressed);
    TEST_ASSERT("after round1 handler clear, the control store commits and both VModule source and trigger slot stay quiet",
        round1_clear_allows_control_path &&
        no_storm_repeat_after_sources_clear);

    TEST_END(ai_interrupt_m_vmodule_meip_storm_breakpoint_rearm_clear_once_corner);
}

bool ai_interrupt_m_vmodule_branch_redirect_execute_trigger_winner_unique_corner()
{
    TEST_START();

    const uintptr_t wrong_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t right_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t wrong_seed = 0x62726e6368777201ULL;
    const uint64_t right_seed = 0x62726e6368727402ULL;
    const uint64_t guard_seed = 0x62726e6368676403ULL;
    const uint64_t after_seed = 0x62726e6368616604ULL;
    const uint64_t right_marker = 0x7269676874ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(wrong_addr, wrong_seed);
    write64(right_addr, right_seed);
    write64(guard_addr, guard_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_branch_redirect_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr, 0ULL);
    write64(wrong_addr, wrong_seed);
    write64(right_addr, right_seed);

    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_branch_wrong_path_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_branch_redirect_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr, 1ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool wrong_path_trigger_flushed_by_branch =
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_branch_wrong_path_site) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_marker &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    write64(right_addr, right_seed);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_branch_right_path_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_branch_redirect_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr, 1ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool right_path_trigger_winner_unique =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_branch_right_path_site) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_branch_wrong_path_site) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_branch_redirect_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr, 1ULL);
    const bool cleared_right_path_commits_without_ghost =
        ai_ibc_trap_count == trap_count_after_clear &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_marker &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_repeat_after_sources_clear =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with slot0 armed on the branch wrong-path execute site, the branch redirect flushes that helper while VModule MEI is still delivered and cleared",
        wrong_path_trigger_flushed_by_branch);
    TEST_ASSERT("after swapping slot0 to the right-path execute site, the visible breakpoint belongs only to the taken path and suppresses the right-path marker store",
        right_path_trigger_winner_unique);
    TEST_ASSERT("after clearing the right-path trigger, the same branch path commits exactly once and neither VModule MEI nor wrong-path breakpoint repeats",
        cleared_right_path_commits_without_ghost &&
        no_late_repeat_after_sources_clear);

    TEST_END(ai_interrupt_m_vmodule_branch_redirect_execute_trigger_winner_unique_corner);
}

bool ai_interrupt_m_vmodule_lsq_younger_store_flush_on_breakpoint_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t older_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t younger_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6c7371666c757301ULL;
    const uint64_t older_seed = 0x6c73716f6c646502ULL;
    const uint64_t younger_seed = 0x6c7371796f756703ULL;
    const uint64_t after_seed = 0x6c73716166747204ULL;
    const uint64_t older_blocked_payload = 0x6f6c646572627010ULL;
    const uint64_t younger_blocked_payload = 0x796f756e67627011ULL;
    const uint64_t older_commit_payload = 0x6f6c646572636d20ULL;
    const uint64_t younger_commit_payload = 0x796f756e67636d21ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(older_addr, older_seed);
    write64(younger_addr, younger_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SKIP_LSQ_YOUNG_ON_BKP;

    ai_ibc_arm_mcontrol6_slot(0, older_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_lsq_store_pair_probe((uint64_t *)older_addr,
        (uint64_t *)younger_addr, older_blocked_payload,
        younger_blocked_payload);
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool event_set_is_precise =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_lsq_store_pair_older_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 1ULL;
    const bool younger_side_effect_does_not_overtake =
        read64(guard_addr) == guard_seed &&
        read64(older_addr) == older_seed &&
        read64(younger_addr) == younger_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_lsq_store_pair_probe((uint64_t *)older_addr,
        (uint64_t *)younger_addr, older_commit_payload,
        younger_commit_payload);
    marker += 1ULL;
    const bool clear_allows_ordered_pair_commit =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(guard_addr) == guard_seed &&
        read64(older_addr) == older_commit_payload &&
        read64(younger_addr) == younger_commit_payload &&
        read64(after_addr) == after_seed &&
        marker == 2ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_interrupt_or_breakpoint_repeat =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule MEI and the older store-address breakpoint are both observed once without assuming which redirect wins first",
        event_set_is_precise);
    TEST_ASSERT("while the older store is stopped by breakpoint, neither the older store nor the younger LSQ marker store becomes visible",
        younger_side_effect_does_not_overtake);
    TEST_ASSERT("after clearing the address trigger and VModule source, the same store pair commits in program order exactly once with no late ghost trap",
        clear_allows_ordered_pair_commit &&
        no_late_interrupt_or_breakpoint_repeat);

    TEST_END(ai_interrupt_m_vmodule_lsq_younger_store_flush_on_breakpoint_corner);
}

bool ai_interrupt_m_vmodule_status_epc_back_to_back_no_mix_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_MEI_OFF_BKP_ON_STATUS;

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_first_status_frame =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_MEI &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPIE) != 0ULL &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const bool interrupt_then_breakpoint_skip_frame =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) != 0ULL &&
        marker == 1ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_MEI_OFF_BKP_ON_STATUS;
    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    const bool meip_held_for_breakpoint_first =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT) &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool breakpoint_then_interrupt_status_frame =
        meip_held_for_breakpoint_first &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPIE) != 0ULL &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 2ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_mixed_status_repeat =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        marker == 2ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule MEI is handled before breakpoint, the interrupt mstatus frame has M-mode xPP/xPIE and the handler's MPIE clear leaves final MIE closed",
        interrupt_first_status_frame);
    TEST_ASSERT("the following breakpoint still skips its own ebreak PC and its MPIE update reopens MIE without inheriting the prior interrupt return frame",
        interrupt_then_breakpoint_skip_frame);
    TEST_ASSERT("when breakpoint opens MIE with a VModule MEIP held pending, the breakpoint PC and the following interrupt status frame remain separate and no late mixed trap repeats",
        breakpoint_then_interrupt_status_frame &&
        no_late_mixed_status_repeat);

    TEST_END(ai_interrupt_m_vmodule_status_epc_back_to_back_no_mix_corner);
}

bool ai_interrupt_hs_vmodule_meip_breakpoint_priv_stack_restore_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_HS_BKP_OPEN_MEI_STACK;

    goto_priv(PRIV_HS);
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const unsigned priv_after_mainline = curr_priv;

    goto_priv(PRIV_M);
    const bool nested_trap_events_are_separate =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_S &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        ai_ibc_trap_epc[0] != ai_ibc_trap_epc[1];
    const bool outer_stack_restored_before_mret =
        ai_ibc_priv_stack_bp_returns == 1ULL &&
        ai_ibc_priv_stack_mei_returns == 1ULL &&
        ai_ibc_priv_stack_outer_mepc ==
            (uintptr_t)ai_ibc_ebreak_probe + 4ULL &&
        (ai_ibc_priv_stack_outer_mstatus & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_S &&
        (ai_ibc_priv_stack_after_mei_mstatus &
            AI_IBC_MSTATUS_MPP_MASK) == AI_IBC_MSTATUS_MPP_M &&
        (ai_ibc_priv_stack_final_mstatus &
            AI_IBC_MSTATUS_MPP_MASK) == AI_IBC_MSTATUS_MPP_S;
    const bool returned_to_hs_mainline_once =
        marker == 1ULL &&
        priv_after_mainline == PRIV_HS &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_MEDELEG, saved_medeleg);

    TEST_ASSERT("an HS/S breakpoint trapped to M records an outer MPP=S frame, while the nested VModule MEI records a separate M-mode interrupt frame",
        nested_trap_events_are_separate);
    TEST_ASSERT("the breakpoint handler preserves the outer mepc/mstatus, lets the nested MEI clear its VModule source, then restores MPP=S before the final mret",
        outer_stack_restored_before_mret);
    TEST_ASSERT("after nested breakpoint plus MEI handling, control returns to the HS/S mainline exactly once with no pending MEIP or ghost trap replay",
        returned_to_hs_mainline_once);

    TEST_END(ai_interrupt_hs_vmodule_meip_breakpoint_priv_stack_restore_corner);
}

bool ai_interrupt_m_vmodule_trap_csr_owner_back_to_back_no_stale_return_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6373726f776e6701ULL;
    const uint64_t target_seed = 0x6373726f776e7402ULL;
    const uint64_t after_seed = 0x6373726f776e6103ULL;
    const uint64_t marker_seed = 0x6373726f776e6d04ULL;
    const uint64_t blocked_payload_a = 0x6373726f776eaa10ULL;
    const uint64_t blocked_payload_b = 0x6373726f776ebb20ULL;
    const uint64_t after_payload_a = 0x6373726f776eac30ULL;
    const uint64_t after_payload_b = 0x6373726f776ebc40ULL;
    const uint64_t marker_payload = 0x6373726f776e6d50ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_store_once(target_addr, blocked_payload_a);
    marker += 0x1ULL;
    ai_ibc_store_once(after_addr, after_payload_a);
    marker += 0x2ULL;
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_then_breakpoint_csr_owner =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_MEI &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_store_once &&
        ai_ibc_trap_tval[0] != target_addr &&
        ai_ibc_trap_tval[1] == target_addr &&
        ai_ibc_trap_epc[0] != ai_ibc_trap_epc[1] &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPIE) != 0ULL &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPIE) != 0ULL &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool interrupt_then_breakpoint_return_contract =
        marker == 0x3ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_payload_a &&
        read64(marker_addr) == marker_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    ai_ibc_reset_log();
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_MEI_OFF_BKP_ON_STATUS;
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    const bool meip_held_before_breakpoint =
        ai_ibc_wait_for_mip_bits(MIP_MEIP, AI_IBC_VMODULE_POLL_LIMIT) &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;
    ai_ibc_store_once(target_addr, blocked_payload_b);
    marker += 0x4ULL;
    ai_ibc_store_once(after_addr, after_payload_b);
    marker += 0x8ULL;
    write64(marker_addr, marker_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool breakpoint_then_interrupt_csr_owner =
        meip_held_before_breakpoint &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_store_once &&
        ai_ibc_trap_tval[0] == target_addr &&
        ai_ibc_trap_tval[1] != target_addr &&
        ai_ibc_trap_epc[0] != ai_ibc_trap_epc[1] &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPIE) == 0ULL &&
        (ai_ibc_trap_mstatus[0] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPIE) != 0ULL &&
        (ai_ibc_trap_mstatus[1] & AI_IBC_MSTATUS_MPP_MASK) ==
            AI_IBC_MSTATUS_MPP_M &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool breakpoint_then_interrupt_return_contract =
        marker == 0xfULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_payload_b &&
        read64(marker_addr) == marker_payload;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_csr_owner_replay =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_payload_b &&
        marker == 0xfULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when VModule MEI is logged immediately before a store-address breakpoint, the two handler-entry CSR snapshots keep independent cause, epc, tval, and mstatus owners",
        interrupt_then_breakpoint_csr_owner);
    TEST_ASSERT("the interrupt-then-breakpoint episode returns through both handlers exactly once: the breakpoint store is suppressed while the following marker store commits",
        interrupt_then_breakpoint_return_contract);
    TEST_ASSERT("when a held VModule MEIP is released by the breakpoint return frame, the breakpoint CSR owner and following interrupt CSR owner do not cross-copy stale epc/tval/status",
        breakpoint_then_interrupt_csr_owner);
    TEST_ASSERT("the breakpoint-then-interrupt episode returns to the post-breakpoint stream exactly once and leaves no late MEI/BKP replay",
        breakpoint_then_interrupt_return_contract &&
        no_late_csr_owner_replay);

    TEST_END(ai_interrupt_m_vmodule_trap_csr_owner_back_to_back_no_stale_return_corner);
}

bool ai_interrupt_m_vmodule_trigger_csr_read_slot_stability_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x7472696763727301ULL;
    const uint64_t target_seed = 0x7472696774617202ULL;
    const uint64_t after_seed = 0x7472696761667403ULL;
    const uint64_t blocked_payload = 0x74726967626c6b10ULL;
    const uint64_t commit_payload = 0x74726967636d7420ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t slot0_tdata1 = 0ULL;
    uint64_t slot0_tdata2 = 0ULL;
    uint64_t slot1_tdata1 = 0ULL;
    uint64_t slot1_tdata2 = 0ULL;
    uint64_t slot0_tselect = 0ULL;
    uint64_t slot1_tselect = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_store_once,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_arm_mcontrol6_slot(1, target_addr, AI_IBC_MCONTROL6_STORE,
        false);

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    slot0_tselect = ai_ibc_read_trigger_csr_slot(0, &slot0_tdata1,
        &slot0_tdata2);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    slot1_tselect = ai_ibc_read_trigger_csr_slot(1, &slot1_tdata1,
        &slot1_tdata2);

    const bool slot_readback_not_cross_mixed =
        slot0_tselect == 0ULL &&
        slot1_tselect == 1ULL &&
        (slot0_tdata1 & AI_IBC_MCONTROL6_EXECUTE) != 0ULL &&
        (slot0_tdata1 & AI_IBC_MCONTROL6_STORE) == 0ULL &&
        slot0_tdata2 == (uintptr_t)ai_ibc_store_once &&
        (slot1_tdata1 & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        (slot1_tdata1 & AI_IBC_MCONTROL6_EXECUTE) == 0ULL &&
        slot1_tdata2 == target_addr &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool final_tselect_is_explicit_slot1 =
        CSRR(CSR_TSELECT) == 1ULL;

    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool execute_slot_still_matches_after_csr_read =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool store_slot_still_matches_after_slot0_clear =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 2ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(1);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, commit_payload);
    const bool both_slots_clear_allows_commit =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == commit_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("reading slot0 tselect/tdata1/tdata2 across a VModule MEI and then reading slot1 keeps each trigger slot's CSR tuple self-consistent",
        slot_readback_not_cross_mixed &&
        final_tselect_is_explicit_slot1);
    TEST_ASSERT("after the interrupted CSR diagnostic read, slot0 still reports the execute-PC breakpoint and suppresses the target store",
        execute_slot_still_matches_after_csr_read);
    TEST_ASSERT("clearing slot0 exposes slot1's store-address breakpoint, and clearing both slots lets the same store commit once",
        store_slot_still_matches_after_slot0_clear &&
        both_slots_clear_allows_commit);

    TEST_END(ai_interrupt_m_vmodule_trigger_csr_read_slot_stability_corner);
}

bool ai_interrupt_m_vmodule_predecode_illegal_execute_trigger_lane_no_swap_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t trigger32_marker = 0ULL;
    volatile uint64_t after32_marker = 0ULL;
    volatile uint64_t illegal16_marker = 0ULL;
    volatile uint64_t trigger16_marker = 0ULL;
    volatile uint64_t after16_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SKIP_ILLEGAL;
    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_lane_trigger32_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_trigger_then_illegal32_probe(&trigger32_marker,
        &after32_marker);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool trigger32_then_illegal32_lane_contract =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_lane_trigger32_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_ILLEGAL_INSTRUCTION,
            (uintptr_t)ai_ibc_lane_illegal32_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_lane_illegal32_site) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_ILLEGAL_INSTRUCTION,
            (uintptr_t)ai_ibc_lane_trigger32_site) == 0ULL &&
        trigger32_marker == 0ULL &&
        after32_marker == 0x696c673332ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_reset_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SKIP_ILLEGAL;
    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_lane_trigger_after_cillegal_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_cillegal16_then_trigger_probe(&illegal16_marker,
        &trigger16_marker, &after16_marker);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool cillegal16_then_trigger_lane_contract =
        ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_ILLEGAL_INSTRUCTION,
            (uintptr_t)ai_ibc_lane_cillegal16_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_lane_trigger_after_cillegal_site) ==
            1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_lane_cillegal16_site) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_ILLEGAL_INSTRUCTION,
            (uintptr_t)ai_ibc_lane_trigger_after_cillegal_site) ==
            0ULL &&
        illegal16_marker == 0x696c673136ULL &&
        trigger16_marker == 0ULL &&
        after16_marker == 0x6166743136ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_replay_or_source_repeat =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        trigger32_marker == 0ULL &&
        after32_marker == 0x696c673332ULL &&
        illegal16_marker == 0x696c673136ULL &&
        trigger16_marker == 0ULL &&
        after16_marker == 0x6166743136ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with a VModule MEI in the window, an execute trigger lane followed by a 32-bit illegal lane records breakpoint and illegal PCs without swapping metadata",
        trigger32_then_illegal32_lane_contract);
    TEST_ASSERT("with the lane order reversed through a 16-bit illegal instruction, the illegal skip and later execute trigger keep separate cause/epc records and marker effects",
        cillegal16_then_trigger_lane_contract);
    TEST_ASSERT("after both predecode-lane episodes, the VModule MEI source is cleared and no late breakpoint, illegal, or marker replay appears",
        no_late_replay_or_source_repeat);

    TEST_END(ai_interrupt_m_vmodule_predecode_illegal_execute_trigger_lane_no_swap_corner);
}

bool ai_interrupt_m_vmodule_frontend_ebreak_lane_length_no_swap_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t wrong_marker = 0ULL;
    volatile uint64_t trigger_marker = 0ULL;
    volatile uint64_t cebreak_marker = 0ULL;
    volatile uint64_t ebreak_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_frontend_lane_wrong_trigger_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_frontend_lane_mixed_probe(&wrong_marker, &trigger_marker,
        &cebreak_marker, &ebreak_marker, 1ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool wrong_path_trigger_flushed =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_wrong_trigger_site) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_right_trigger_site) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_cebreak_site) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_ebreak_site) == 1ULL &&
        wrong_marker == 0ULL &&
        trigger_marker == 1ULL &&
        cebreak_marker == 0x16ULL &&
        ebreak_marker == 0x32ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    wrong_marker = 0ULL;
    trigger_marker = 0ULL;
    cebreak_marker = 0ULL;
    ebreak_marker = 0ULL;
    ai_ibc_reset_log();
    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_frontend_lane_right_trigger_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_frontend_lane_mixed_probe(&wrong_marker, &trigger_marker,
        &cebreak_marker, &ebreak_marker, 1ULL);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 3ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool right_path_trigger_and_breakpoints_precise =
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 3ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_wrong_trigger_site) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_right_trigger_site) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_cebreak_site) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_frontend_lane_ebreak_site) == 1ULL &&
        wrong_marker == 0ULL &&
        trigger_marker == 0ULL &&
        cebreak_marker == 0x16ULL &&
        ebreak_marker == 0x32ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_lane_or_source_replay =
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 3ULL &&
        wrong_marker == 0ULL &&
        trigger_marker == 0ULL &&
        cebreak_marker == 0x16ULL &&
        ebreak_marker == 0x32ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with a VModule MEI in the frontend window, an execute trigger armed on the flushed wrong-path lane does not create a ghost breakpoint while c.ebreak and ebreak keep their own PCs",
        wrong_path_trigger_flushed);
    TEST_ASSERT("when the execute trigger is moved to the right-path lane, its skipped store, the later c.ebreak, and the split 32-bit ebreak are logged as three separate breakpoint PCs",
        right_path_trigger_and_breakpoints_precise);
    TEST_ASSERT("after the mixed-lane sequence returns, the VModule MEI source is cleared and no stale lane metadata or marker replay appears",
        no_late_lane_or_source_replay);

    TEST_END(ai_interrupt_m_vmodule_frontend_ebreak_lane_length_no_swap_corner);
}

bool ai_interrupt_m_vmodule_indirect_predictor_wrong_path_no_ghost_breakpoint_corner()
{
    TEST_START();

    const uintptr_t wrong_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t right_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t wrong_seed = 0x696e647772736501ULL;
    const uint64_t right_seed = 0x696e647274736502ULL;
    const uint64_t guard_seed = 0x696e646775617203ULL;
    const uint64_t after_seed = 0x696e646166747204ULL;
    const uint64_t wrong_marker = 0x696e646972777201ULL;
    const uint64_t right_marker = 0x696e646972727401ULL;
    uint64_t trained_wrong_hits = 0ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(wrong_addr, wrong_seed);
    write64(right_addr, right_seed);
    write64(guard_addr, guard_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    for (uint64_t i = 0; i < 6ULL; ++i) {
        ai_ibc_indirect_predict_probe((uint64_t *)wrong_addr,
            (uint64_t *)right_addr,
            (uintptr_t)ai_ibc_indirect_wrong_target);
        if (read64(wrong_addr) == wrong_marker &&
            read64(right_addr) == right_seed) {
            ++trained_wrong_hits;
        }
        write64(wrong_addr, wrong_seed);
        write64(right_addr, right_seed);
    }
    const bool predictor_training_path_observed =
        trained_wrong_hits == 6ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_indirect_wrong_store_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_indirect_predict_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr,
        (uintptr_t)ai_ibc_indirect_right_target);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool wrong_path_has_no_ghost_breakpoint =
        predictor_training_path_observed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_indirect_wrong_store_site) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_marker &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    write64(wrong_addr, wrong_seed);
    write64(right_addr, right_seed);

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_indirect_right_store_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_indirect_predict_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr,
        (uintptr_t)ai_ibc_indirect_right_target);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool right_path_breakpoint_remains_visible =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_indirect_right_store_site) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_indirect_wrong_store_site) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_seed &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_indirect_predict_probe((uint64_t *)wrong_addr,
        (uint64_t *)right_addr,
        (uintptr_t)ai_ibc_indirect_right_target);
    const bool cleared_right_path_commits_without_repeat =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_marker &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_predictor_or_interrupt_repeat =
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(wrong_addr) == wrong_seed &&
        read64(right_addr) == right_marker;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("after repeatedly training the indirect branch toward a wrong target, VModule MEI plus the real right-target jump does not expose a wrong-path execute breakpoint",
        wrong_path_has_no_ghost_breakpoint);
    TEST_ASSERT("when the execute trigger is armed on the actual right-path target, the breakpoint remains visible and suppresses only the right-path marker store",
        right_path_breakpoint_remains_visible);
    TEST_ASSERT("after clearing the right-path trigger, the same indirect jump commits once and no late VModule interrupt or predictor ghost breakpoint repeats",
        cleared_right_path_commits_without_repeat &&
        no_late_predictor_or_interrupt_repeat);

    TEST_END(ai_interrupt_m_vmodule_indirect_predictor_wrong_path_no_ghost_breakpoint_corner);
}

bool ai_interrupt_m_vmodule_store_address_trigger_late_cancel_no_payload_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x73746c6370676401ULL;
    const uint64_t target_seed = 0x73746c6374677402ULL;
    const uint64_t after_seed = 0x73746c6361667403ULL;
    const uint64_t marker_seed = 0x73746c636d6b7204ULL;
    const uint64_t blocked_payload = 0x73746c63626c6b10ULL;
    const uint64_t committed_payload = 0x73746c63636d7420ULL;
    const uint64_t second_payload = 0x73746c63666e6c30ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool breakpoint_cancel_keeps_memory_image =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, committed_payload);
    const bool clear_trigger_allows_exact_store_commit =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_store_once(target_addr, second_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rearmed_store_cancel_has_no_late_payload =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_store_once) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in the store-address trigger window, the breakpoint records the target address and the blocked store payload never reaches memory",
        breakpoint_cancel_keeps_memory_image);
    TEST_ASSERT("after clearing the store-address trigger and VModule source, the same target store commits exactly once without an extra trap",
        clear_trigger_allows_exact_store_commit);
    TEST_ASSERT("after rearming the trigger and reinjecting MEI, a second blocked store again leaves no late store payload or repeated interrupt source",
        rearmed_store_cancel_has_no_late_payload);

    TEST_END(ai_interrupt_m_vmodule_store_address_trigger_late_cancel_no_payload_corner);
}

bool ai_interrupt_hs_vmodule_pbmt_nc_store_breakpoint_mei_profile_guard_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt0;
    const uintptr_t target_paddr = phys_page_base(X) + 0x280U;
    const uintptr_t guard_paddr = target_paddr + sizeof(uint64_t);
    const uint64_t target_seed = 0x50424d544e435401ULL;
    const uint64_t guard_seed = 0x50424d544e434702ULL;
    const uint64_t blocked_payload = 0x50424d544e434203ULL;
    const uint64_t commit_payload = 0x50424d544e434304ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    volatile uint64_t marker = 0ULL;

    const uintptr_t target_vaddr =
        ai_ibc_prepare_pbmt_nc_alias_to_paddr(nc_page, target_paddr);
    const uintptr_t guard_vaddr = target_vaddr + sizeof(uint64_t);
    const bool profile_mem_nc_dram_subset =
        target_paddr >= 0x80000000ULL &&
        target_paddr < 0x2000000000ULL &&
        guard_paddr >= 0x80000000ULL &&
        guard_paddr < 0x2000000000ULL;

    goto_priv(PRIV_HS);
    ai_ibc_store_once(target_vaddr, target_seed);
    ai_ibc_store_once(guard_vaddr, guard_seed);
    const uint64_t alias_seed_target = ai_ibc_load_once(target_vaddr);
    const uint64_t alias_seed_guard = ai_ibc_load_once(guard_vaddr);
    goto_priv(PRIV_M);

    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    ai_ibc_arm_mcontrol6_slot_priv(0, target_vaddr,
        AI_IBC_MCONTROL6_STORE, AI_IBC_MCONTROL6_S, false);

    ai_ibc_prepare_vmodule_meip_pending(true, 2ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    goto_priv(PRIV_HS);
    ai_ibc_store_once(target_vaddr, blocked_payload);
    marker += 1ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_after_events = CSRR(CSR_MIP);
    const uint64_t trap_count_after_events = ai_ibc_trap_count;

    goto_priv(PRIV_HS);
    const uint64_t alias_after_blocked_target = ai_ibc_load_once(target_vaddr);
    const uint64_t alias_after_blocked_guard = ai_ibc_load_once(guard_vaddr);
    goto_priv(PRIV_M);

    const bool pbmt_nc_store_breakpoint_and_mei_contract =
        profile_mem_nc_dram_subset &&
        alias_seed_target == target_seed &&
        alias_seed_guard == guard_seed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_vaddr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (mip_after_events & MIP_MEIP) == 0ULL &&
        alias_after_blocked_target == target_seed &&
        alias_after_blocked_guard == guard_seed &&
        marker == 1ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    goto_priv(PRIV_HS);
    const uint64_t alias_after_spin_target = ai_ibc_load_once(target_vaddr);
    const uint64_t alias_after_spin_guard = ai_ibc_load_once(guard_vaddr);
    goto_priv(PRIV_M);

    const bool no_late_replay_or_source_reassert =
        ai_ibc_trap_count == trap_count_after_events &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        alias_after_spin_target == target_seed &&
        alias_after_spin_guard == guard_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_clear_main_interrupt();
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    goto_priv(PRIV_HS);
    ai_ibc_store_once(target_vaddr, commit_payload);
    marker += 1ULL;
    const uint64_t alias_after_commit_target = ai_ibc_load_once(target_vaddr);
    const uint64_t alias_after_commit_guard = ai_ibc_load_once(guard_vaddr);

    goto_priv(PRIV_M);
    const bool clearing_trigger_allows_pbmt_nc_store_commit =
        ai_ibc_trap_count == trap_count_after_events &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        alias_after_commit_target == commit_payload &&
        alias_after_commit_guard == guard_seed &&
        marker == 2ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, MIE_MEIE);
    CSRC(CSR_MIP, MIP_MEIP);
    CSRC(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("profile guard selects only the PMA=MEM plus PBMT=NC DRAM subset, then a HS/S PBMT=NC store-address breakpoint and VModule MEI are both observed exactly once",
        pbmt_nc_store_breakpoint_and_mei_contract,
        "target_pa=%lx target_va=%lx guard_va=%lx count=%lx bkp=%lx mei=%lx mip=%lx blocked=%lx guard=%lx marker=%lx",
        target_paddr, target_vaddr, guard_vaddr, trap_count_after_events,
        ai_ibc_log_count_cause(CAUSE_BKP),
        ai_ibc_log_count_cause(CAUSE_MEI), mip_after_events,
        alias_after_blocked_target, alias_after_blocked_guard, marker);
    TEST_ASSERT("after the PBMT=NC breakpoint and VModule MEI return, vm_reg_0.MEIP stays cleared and no stale breakpoint or store side effect replays",
        no_late_replay_or_source_reassert);
    TEST_ASSERT("after clearing the store-address trigger and VModule source, the same PBMT=NC alias store commits once while the adjacent backing word is preserved",
        clearing_trigger_allows_pbmt_nc_store_commit);

    TEST_END(ai_interrupt_hs_vmodule_pbmt_nc_store_breakpoint_mei_profile_guard_corner);
}

bool ai_interrupt_m_vmodule_frontend_icache_refill_old_block_no_ghost_breakpoint_corner()
{
    TEST_START();

    const uintptr_t old_marker_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t right_marker_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t old_seed = 0x696372666f6c6401ULL;
    const uint64_t right_seed = 0x6963726672677402ULL;
    const uint64_t guard_seed = 0x6963726667756403ULL;
    const uint64_t after_seed = 0x6963726661667404ULL;
    const uint64_t old_payload = 0x696372666f6c6410ULL;
    const uint64_t right_payload = 0x6963726672677420ULL;
    const uint64_t old_revisit_payload = 0x696372666f6c6430ULL;
    const uint64_t final_old_payload = 0x696372666f6c6440ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(old_marker_addr, old_seed);
    write64(right_marker_addr, right_seed);
    write64(guard_addr, guard_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_arm_mcontrol6_slot(0,
        (uintptr_t)ai_ibc_icache_refill_old_block_site,
        AI_IBC_MCONTROL6_EXECUTE, false);

    ai_ibc_fence_i();
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_icache_refill_cross_probe((volatile uint64_t *)old_marker_addr,
        (volatile uint64_t *)right_marker_addr,
        1ULL,
        old_payload,
        right_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_flush_does_not_replay_old_block =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_icache_refill_old_block_site) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(old_marker_addr) == old_seed &&
        read64(right_marker_addr) == right_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_clear_main_interrupt();
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_icache_refill_cross_probe((volatile uint64_t *)old_marker_addr,
        (volatile uint64_t *)right_marker_addr,
        0ULL,
        old_revisit_payload,
        right_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool explicit_old_block_still_hits_current_breakpoint =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_icache_refill_old_block_site) == 1ULL &&
        read64(old_marker_addr) == old_seed &&
        read64(right_marker_addr) == right_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_trigger_clear = ai_ibc_trap_count;
    ai_ibc_icache_refill_cross_probe((volatile uint64_t *)old_marker_addr,
        (volatile uint64_t *)right_marker_addr,
        0ULL,
        final_old_payload,
        right_payload);
    const bool clearing_trigger_allows_old_block_commit =
        ai_ibc_trap_count == trap_count_after_trigger_clear &&
        read64(old_marker_addr) == final_old_payload &&
        read64(right_marker_addr) == right_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_interrupt_or_fetch_metadata_replay =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(old_marker_addr) == final_old_payload &&
        read64(right_marker_addr) == right_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in the frontend old/right block window, the right block commits once and the old-block execute trigger is not replayed as a ghost breakpoint",
        interrupt_flush_does_not_replay_old_block,
        "count=%lx mei=%lx bkp=%lx old=%lx right=%lx mip=%lx",
        ai_ibc_trap_count, ai_ibc_log_count_cause(CAUSE_MEI),
        ai_ibc_log_count_cause(CAUSE_BKP), read64(old_marker_addr),
        read64(right_marker_addr), CSRR(CSR_MIP));
    TEST_ASSERT("explicitly revisiting the old fetch block while the trigger remains armed reports the current old-block breakpoint and suppresses its marker store",
        explicit_old_block_still_hits_current_breakpoint);
    TEST_ASSERT("after clearing the execute trigger and VModule MEI source, the old block commits once and no late interrupt or stale fetch metadata replays",
        clearing_trigger_allows_old_block_commit &&
        no_late_interrupt_or_fetch_metadata_replay);

    TEST_END(ai_interrupt_m_vmodule_frontend_icache_refill_old_block_no_ghost_breakpoint_corner);
}

bool ai_interrupt_m_vmodule_load_address_trigger_late_response_no_rd_leak_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t load_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6c64727370676401ULL;
    const uint64_t load_payload = 0x6c64727370617902ULL;
    const uint64_t marker_seed = 0x6c6472736d6b7203ULL;
    const uint64_t after_seed = 0x6c64727361667404ULL;
    const uint64_t blocked_sentinel = 0x6c64727373656e10ULL;
    const uint64_t clean_sentinel = 0x6c64727373656e20ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(load_addr, load_payload);
    write64(marker_addr, marker_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, load_addr, AI_IBC_MCONTROL6_LOAD,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_load_dependent_store_probe((uint64_t *)load_addr,
        (uint64_t *)marker_addr, blocked_sentinel);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool load_trigger_blocks_rd_payload =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_load_dependent_store_load_site) ==
            1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, load_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(marker_addr) == blocked_sentinel &&
        read64(load_addr) == load_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_load_dependent_store_probe((uint64_t *)load_addr,
        (uint64_t *)marker_addr, clean_sentinel);
    const bool clear_trigger_allows_payload_writeback =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(marker_addr) == load_payload &&
        read64(load_addr) == load_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    write64(marker_addr, marker_seed);
    ai_ibc_arm_mcontrol6_slot(0, load_addr, AI_IBC_MCONTROL6_LOAD,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_load_dependent_store_probe((uint64_t *)load_addr,
        (uint64_t *)marker_addr, clean_sentinel);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rearmed_load_has_no_late_rd_leak =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_load_dependent_store_load_site) ==
            2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(marker_addr) == clean_sentinel &&
        read64(load_addr) == load_payload &&
        read64(guard_addr) == guard_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in the load-address trigger window, the breakpoint records the load address and the dependent marker never receives the loaded payload",
        load_trigger_blocks_rd_payload);
    TEST_ASSERT("after clearing the load-address trigger and VModule source, the same load writes the expected payload into the dependent marker exactly once",
        clear_trigger_allows_payload_writeback);
    TEST_ASSERT("after rearming the load trigger and reinjecting MEI, no late load response leaks into the dependent marker and the interrupt source is cleared",
        rearmed_load_has_no_late_rd_leak);

    TEST_END(ai_interrupt_m_vmodule_load_address_trigger_late_response_no_rd_leak_corner);
}

bool ai_interrupt_m_vmodule_meip_stip_csr_write_arbitration_no_cross_clear_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6d73726367756401ULL;
    const uint64_t target_seed = 0x6d73726374677402ULL;
    const uint64_t after_seed = 0x6d73726361667403ULL;
    const uint64_t marker_seed = 0x6d7372636d6b7204ULL;
    const uint64_t blocked_payload = 0x6d737263626c6b10ULL;
    const uint64_t committed_payload = 0x6d737263636d7420ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE | MIE_MEIE);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    for (uint64_t i = 0; i < AI_IBC_STIP_WRITE_ROUNDS / 2ULL; ++i) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_prepare_vmodule_meip_pending(false, 8ULL);
    uint64_t both_pending_before_breakpoint = 0ULL;
    const uint64_t pending_rounds =
        ai_ibc_write_stip_until_meip_pending(
            AI_IBC_VMODULE_POLL_LIMIT,
            &both_pending_before_breakpoint);

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_CLEAR_STIP_ON_BKP;
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_store_once(target_addr, blocked_payload);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);

    const bool breakpoint_clears_only_stip =
        pending_rounds > 0ULL &&
        pending_rounds < AI_IBC_VMODULE_POLL_LIMIT &&
        (both_pending_before_breakpoint &
            (AI_IBC_MIP_STIP | MIP_MEIP)) ==
            (AI_IBC_MIP_STIP | MIP_MEIP) &&
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_store_once &&
        (ai_ibc_trap_mip[0] & (AI_IBC_MIP_STIP | MIP_MEIP)) ==
            (AI_IBC_MIP_STIP | MIP_MEIP) &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) == 0ULL &&
        (mip_after_breakpoint & MIP_MEIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    uint64_t stip_rewrite_observed = 0ULL;
    for (uint64_t i = 0; i < AI_IBC_STIP_WRITE_ROUNDS; ++i) {
        CSRS(CSR_MIP, AI_IBC_MIP_STIP);
        if ((CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL) {
            ++stip_rewrite_observed;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_after_stip_rewrite = CSRR(CSR_MIP);
    const bool repeated_stip_write_keeps_meip_owned =
        stip_rewrite_observed == AI_IBC_STIP_WRITE_ROUNDS &&
        (mip_after_stip_rewrite & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_stip_rewrite & MIP_MEIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL &&
        read64(target_addr) == target_seed;

    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_after_mei = CSRR(CSR_MIP);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mei_handler_clears_only_vmodule_source =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_MEI &&
        (ai_ibc_trap_mip[1] & (AI_IBC_MIP_STIP | MIP_MEIP)) ==
            (AI_IBC_MIP_STIP | MIP_MEIP) &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (mip_after_mei & MIP_MEIP) == 0ULL &&
        (mip_after_mei & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mip_after_sti = CSRR(CSR_MIP);
    const bool stip_handler_clears_last_source_once =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[2] == CAUSE_STI &&
        (ai_ibc_trap_mip[2] & AI_IBC_MIP_STIP) != 0ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (mip_after_sti & (AI_IBC_MIP_STIP | MIP_MEIP)) == 0ULL &&
        read64(target_addr) == target_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_trigger_clear = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, committed_payload);
    const bool trigger_clear_allows_exact_store_commit =
        ai_ibc_trap_count == trap_count_after_trigger_clear &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with repeated mip.STIP writes and VModule MEIP both pending, the store breakpoint handler clears only STIP and suppresses only the trapped store",
        breakpoint_clears_only_stip);
    TEST_ASSERT("after the breakpoint clears STIP, repeated CSR writes can restore STIP while the VModule MEIP source remains owned by the MEI handler",
        repeated_stip_write_keeps_meip_owned);
    TEST_ASSERT("opening only MEIE delivers one MEI that clears vm_reg_0.MEIP but preserves the rewritten STIP pending bit",
        mei_handler_clears_only_vmodule_source);
    TEST_ASSERT("opening STIE later delivers the preserved STIP exactly once, then clearing the trigger lets the store commit exactly once",
        stip_handler_clears_last_source_once &&
        trigger_clear_allows_exact_store_commit);

    TEST_END(ai_interrupt_m_vmodule_meip_stip_csr_write_arbitration_no_cross_clear_corner);
}

bool ai_interrupt_m_stip_amo_execute_trigger_rmw_atomic_contract_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t amo_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t before_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x616d6f7374696701ULL;
    const uint64_t amo_seed = 0x0000000020003000ULL;
    const uint64_t before_seed = 0x616d6f7374696202ULL;
    const uint64_t after_seed = 0x616d6f7374696103ULL;
    const uint64_t before_blocked = 0x616d6f7374621010ULL;
    const uint64_t before_commit = 0x616d6f7374632020ULL;
    const uint64_t blocked_addend = 0x44ULL;
    const uint64_t commit_addend = 0x55ULL;
    const uint64_t commit_value = amo_seed + commit_addend;
    const uint64_t blocked_sentinel = 0x616d6f7374733030ULL;
    const uint64_t commit_sentinel = 0x616d6f7374734040ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(amo_addr, amo_seed);
    write64(before_addr, before_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_amo_aqrl_order_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_stip_pending(false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t pending_before_breakpoint = CSRR(CSR_MIP);
    const uint64_t blocked_old =
        ai_ibc_amo_aqrl_order_probe((uint64_t *)amo_addr,
            (uint64_t *)before_addr,
            (uint64_t *)after_addr,
            before_blocked,
            blocked_addend,
            blocked_sentinel);
    const uint64_t pending_after_breakpoint = CSRR(CSR_MIP);

    const bool breakpoint_suppresses_partial_rmw =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_amo_aqrl_order_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (pending_before_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (pending_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        blocked_old == blocked_sentinel &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == amo_seed &&
        read64(before_addr) == before_blocked &&
        read64(after_addr) == blocked_sentinel;

    const uint64_t trap_count_after_breakpoint = ai_ibc_trap_count;
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_after_breakpoint_preserves_memory =
        ai_ibc_trap_count == trap_count_after_breakpoint + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == amo_seed &&
        read64(before_addr) == before_blocked &&
        read64(after_addr) == blocked_sentinel;

    ai_ibc_clear_mcontrol6_slot(0);
    write64(before_addr, before_seed);
    write64(after_addr, after_seed);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    const uint64_t commit_old =
        ai_ibc_amo_aqrl_order_probe((uint64_t *)amo_addr,
            (uint64_t *)before_addr,
            (uint64_t *)after_addr,
            before_commit,
            commit_addend,
            commit_sentinel);
    const bool clear_trigger_retries_atomic_pair =
        ai_ibc_trap_count == trap_count_after_clear &&
        commit_old == amo_seed &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == commit_value &&
        read64(before_addr) == before_commit &&
        read64(after_addr) == amo_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("a pending-disabled STIP does not let an AMO execute-trigger breakpoint perform a partial RMW or rd old-value update",
        breakpoint_suppresses_partial_rmw);
    TEST_ASSERT("after the AMO breakpoint returns, enabling STIE/MIE delivers the preserved timer interrupt once without changing AMO target or markers",
        interrupt_after_breakpoint_preserves_memory);
    TEST_ASSERT("after clearing the AMO trigger, the same amoadd.d.aqrl commits exactly once with old-value and final memory paired",
        clear_trigger_retries_atomic_pair);

    TEST_END(ai_interrupt_m_stip_amo_execute_trigger_rmw_atomic_contract_corner);
}

bool ai_interrupt_m_stip_lrsc_sc_execute_trigger_status_memory_consistent_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6c72736373746901ULL;
    const uint64_t target_seed = 0x6c72736373746902ULL;
    const uint64_t other_seed = 0x6c72736373746903ULL;
    const uint64_t marker_seed = 0x6c72736373746904ULL;
    const uint64_t blocked_sc_payload = 0x6c72736373746210ULL;
    const uint64_t second_sc_payload = 0x6c72736373747320ULL;
    const uint64_t fresh_sc_payload = 0x6c72736373746630ULL;
    const uint64_t blocked_status_sentinel = 0x6c72736373746e40ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(other_addr, other_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, (uintptr_t)ai_ibc_sc_d_status_site,
        AI_IBC_MCONTROL6_EXECUTE, false);
    ai_ibc_prepare_stip_pending(false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t lr_old = ai_ibc_lr_d_once(target_addr);
    const uint64_t blocked_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            blocked_sc_payload,
            blocked_status_sentinel);
    const uint64_t pending_after_breakpoint = CSRR(CSR_MIP);

    const bool breakpoint_suppresses_sc =
        lr_old == target_seed &&
        blocked_sc_status == blocked_status_sentinel &&
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_sc_d_status_site) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (pending_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    const uint64_t trap_count_after_breakpoint = ai_ibc_trap_count;
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_preserves_lrsc_image =
        ai_ibc_trap_count == trap_count_after_breakpoint + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    const uint64_t second_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            second_sc_payload,
            1ULL);
    const uint64_t target_after_second_sc = read64(target_addr);
    const bool second_sc_status_memory_consistent =
        ai_ibc_trap_count == trap_count_after_clear &&
        ((second_sc_status == 0ULL &&
            target_after_second_sc == second_sc_payload) ||
         (second_sc_status != 0ULL &&
            target_after_second_sc == target_seed)) &&
        read64(guard_addr) == guard_seed &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    write64(target_addr, target_seed);
    const uint64_t fresh_lr_old = ai_ibc_lr_d_once(target_addr);
    const uint64_t fresh_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            fresh_sc_payload,
            1ULL);
    const bool fresh_lrsc_recovers_cleanly =
        fresh_lr_old == target_seed &&
        fresh_sc_status == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == fresh_sc_payload &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but locally disabled, an execute trigger on sc.d skips the SC side effect and preserves target memory",
        breakpoint_suppresses_sc);
    TEST_ASSERT("after the trapped SC returns, enabling STIE/MIE delivers the preserved timer interrupt once without modifying LR/SC memory image",
        interrupt_preserves_lrsc_image);
    TEST_ASSERT("after clearing the SC trigger, the next SC status and target memory are self-consistent, and a fresh LR/SC pair recovers cleanly",
        second_sc_status_memory_consistent &&
        fresh_lrsc_recovers_cleanly);

    TEST_END(ai_interrupt_m_stip_lrsc_sc_execute_trigger_status_memory_consistent_corner);
}

bool ai_interrupt_m_vmodule_amo_aqrl_address_trigger_suppresses_order_side_effect_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t amo_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t before_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x616d6f6f72646701ULL;
    const uint64_t amo_seed = 0x0000000010002000ULL;
    const uint64_t before_seed = 0x616d6f6f72646202ULL;
    const uint64_t after_seed = 0x616d6f6f72646103ULL;
    const uint64_t before_blocked0 = 0x616d6f6265663010ULL;
    const uint64_t before_commit = 0x616d6f6265663020ULL;
    const uint64_t before_blocked1 = 0x616d6f6265663030ULL;
    const uint64_t blocked_addend0 = 0x111ULL;
    const uint64_t committed_addend = 0x222ULL;
    const uint64_t blocked_addend1 = 0x333ULL;
    const uint64_t committed_value = amo_seed + committed_addend;
    const uint64_t sentinel0 = 0x616d6f73656e4010ULL;
    const uint64_t sentinel1 = 0x616d6f73656e4020ULL;
    const uint64_t sentinel2 = 0x616d6f73656e4030ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(amo_addr, amo_seed);
    write64(before_addr, before_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, amo_addr, AI_IBC_MCONTROL6_AMO,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t blocked_old0 =
        ai_ibc_amo_aqrl_order_probe((uint64_t *)amo_addr,
            (uint64_t *)before_addr,
            (uint64_t *)after_addr,
            before_blocked0,
            blocked_addend0,
            sentinel0);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool blocked_amo_has_no_order_side_effect =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_amo_aqrl_order_site) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, amo_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        blocked_old0 == sentinel0 &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == amo_seed &&
        read64(before_addr) == before_blocked0 &&
        read64(after_addr) == sentinel0;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    write64(before_addr, before_seed);
    write64(after_addr, after_seed);
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    const uint64_t commit_old =
        ai_ibc_amo_aqrl_order_probe((uint64_t *)amo_addr,
            (uint64_t *)before_addr,
            (uint64_t *)after_addr,
            before_commit,
            committed_addend,
            sentinel1);
    const bool clear_trigger_commits_amo_once =
        ai_ibc_trap_count == trap_count_after_clear &&
        commit_old == amo_seed &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == committed_value &&
        read64(before_addr) == before_commit &&
        read64(after_addr) == amo_seed;

    write64(before_addr, before_seed);
    write64(after_addr, after_seed);
    ai_ibc_arm_mcontrol6_slot(0, amo_addr, AI_IBC_MCONTROL6_AMO,
        false);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    const uint64_t blocked_old1 =
        ai_ibc_amo_aqrl_order_probe((uint64_t *)amo_addr,
            (uint64_t *)before_addr,
            (uint64_t *)after_addr,
            before_blocked1,
            blocked_addend1,
            sentinel2);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rearmed_amo_still_suppresses_late_effects =
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_amo_aqrl_order_site) == 2ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, amo_addr) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        blocked_old1 == sentinel2 &&
        read64(guard_addr) == guard_seed &&
        read64(amo_addr) == committed_value &&
        read64(before_addr) == before_blocked1 &&
        read64(after_addr) == sentinel2;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in the AMO address-trigger window, the breakpoint skips amoadd.d.aqrl and leaves the target and rd old-value marker untouched",
        blocked_amo_has_no_order_side_effect);
    TEST_ASSERT("after clearing the AMO address trigger and VModule source, the same amoadd.d.aqrl commits exactly once with the expected old-value marker",
        clear_trigger_commits_amo_once);
    TEST_ASSERT("after rearming the AMO trigger and reinjecting MEI, a second blocked AMO again leaves no late target update or ordering marker leak",
        rearmed_amo_still_suppresses_late_effects);

    TEST_END(ai_interrupt_m_vmodule_amo_aqrl_address_trigger_suppresses_order_side_effect_corner);
}

bool ai_interrupt_m_vmodule_lrsc_sc_breakpoint_no_stale_success_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t other_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x6c72736367756401ULL;
    const uint64_t target_seed = 0x6c72736374677402ULL;
    const uint64_t other_seed = 0x6c7273636f746803ULL;
    const uint64_t marker_seed = 0x6c7273636d6b7204ULL;
    const uint64_t blocked_sc_payload = 0x6c727363626c6b10ULL;
    const uint64_t stale_sc_payload = 0x6c72736373746120ULL;
    const uint64_t final_sc_payload = 0x6c72736366696e30ULL;
    const uint64_t consumed_sc_payload = 0x6c727363636f6e40ULL;
    const uint64_t blocked_status_sentinel = 0x6c72736373656e50ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(other_addr, other_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t reserved_old = ai_ibc_lr_d_once(target_addr);
    const uint64_t blocked_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            blocked_sc_payload,
            blocked_status_sentinel);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool blocked_sc_does_not_write_memory =
        reserved_old == target_seed &&
        blocked_sc_status == blocked_status_sentinel &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_sc_d_status_site) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t other_reserved = ai_ibc_lr_d_once(other_addr);
    const uint64_t stale_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            stale_sc_payload,
            0ULL);
    const bool stale_success_not_left_on_original_address =
        other_reserved == other_seed &&
        stale_sc_status != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    const uint64_t fresh_reserved = ai_ibc_lr_d_once(target_addr);
    const uint64_t final_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            final_sc_payload,
            1ULL);
    const bool fresh_lrsc_recovers_exactly_once =
        fresh_reserved == target_seed &&
        final_sc_status == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_sc_payload &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed;

    const uint64_t consumed_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            consumed_sc_payload,
            0ULL);
    const bool successful_sc_consumes_reservation =
        consumed_status != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_sc_payload &&
        read64(other_addr) == other_seed &&
        read64(marker_addr) == marker_seed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in the SC store-trigger window, the breakpoint records the sc.d address and the conditional store leaves all memory sentinels unchanged",
        blocked_sc_does_not_write_memory);
    TEST_ASSERT("after replacing the reservation with a different LR address, a naked SC to the original target fails and proves no stale success survived the breakpoint/interrupt episode",
        stale_success_not_left_on_original_address);
    TEST_ASSERT("a fresh LR/SC pair after clearing trigger and VModule source succeeds exactly once, and a second naked SC then fails",
        fresh_lrsc_recovers_exactly_once &&
        successful_sc_consumes_reservation);

    TEST_END(ai_interrupt_m_vmodule_lrsc_sc_breakpoint_no_stale_success_corner);
}

bool ai_interrupt_m_vmodule_lrsc_sc_blocked_no_stale_store_intent_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t neighbor_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t marker_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x73746f7265676401ULL;
    const uint64_t target_seed = 0x73746f7265746702ULL;
    const uint64_t neighbor_seed = 0x73746f72656e6203ULL;
    const uint64_t marker_seed = 0x73746f72656d6b04ULL;
    const uint64_t blocked_sc_payload = 0x73746f7265626c10ULL;
    const uint64_t neighbor_payload = 0x73746f72656e7020ULL;
    const uint64_t marker_payload = 0x73746f72656d7030ULL;
    const uint64_t second_sc_payload = 0x73746f7265736331ULL;
    const uint64_t final_sc_payload = 0x73746f7265666e40ULL;
    const uint64_t blocked_status_sentinel = 0x73746f7265736e50ULL;
    const uint64_t second_status_sentinel = 0x73746f7265736e51ULL;
    const uint64_t final_status_sentinel = 0x73746f7265736e52ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(neighbor_addr, neighbor_seed);
    write64(marker_addr, marker_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(true, 1ULL);
    const uint64_t reserved_old = ai_ibc_lr_d_once(target_addr);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    const uint64_t blocked_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            blocked_sc_payload,
            blocked_status_sentinel);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool blocked_sc_suppressed_store_intent =
        reserved_old == target_seed &&
        blocked_sc_status == blocked_status_sentinel &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_sc_d_status_site) == 1ULL &&
        ai_ibc_log_count_cause_tval(CAUSE_BKP, target_addr) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(neighbor_addr) == neighbor_seed &&
        read64(marker_addr) == marker_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_block = ai_ibc_trap_count;
    ai_ibc_store_once(neighbor_addr, neighbor_payload);
    const uint64_t target_after_neighbor_store = read64(target_addr);
    const uint64_t neighbor_after_store = read64(neighbor_addr);
    ai_ibc_store_once(marker_addr, marker_payload);
    const bool unrelated_store_load_does_not_observe_blocked_payload =
        ai_ibc_trap_count == trap_count_after_block &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(guard_addr) == guard_seed &&
        target_after_neighbor_store == target_seed &&
        target_after_neighbor_store != blocked_sc_payload &&
        neighbor_after_store == neighbor_payload &&
        read64(marker_addr) == marker_payload &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t trap_count_before_second_sc = ai_ibc_trap_count;
    const uint64_t second_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            second_sc_payload,
            second_status_sentinel);
    const uint64_t target_after_second_sc = read64(target_addr);
    const bool second_sc_status_matches_memory =
        ai_ibc_trap_count == trap_count_before_second_sc &&
        ((second_sc_status == 0ULL &&
            target_after_second_sc == second_sc_payload) ||
         (second_sc_status != 0ULL &&
            target_after_second_sc == target_seed)) &&
        target_after_second_sc != blocked_sc_payload &&
        read64(guard_addr) == guard_seed &&
        read64(neighbor_addr) == neighbor_payload &&
        read64(marker_addr) == marker_payload;

    const uint64_t value_before_fresh_lrsc = read64(target_addr);
    const uint64_t fresh_reserved = ai_ibc_lr_d_once(target_addr);
    const uint64_t final_sc_status =
        ai_ibc_sc_d_status_probe((uint64_t *)target_addr,
            final_sc_payload,
            final_status_sentinel);
    const bool fresh_lrsc_commits_after_store_intent_probe =
        fresh_reserved == value_before_fresh_lrsc &&
        final_sc_status == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == final_sc_payload &&
        read64(neighbor_addr) == neighbor_payload &&
        read64(marker_addr) == marker_payload;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_sc_payload_or_trap_replay =
        ai_ibc_trap_count == trap_count_before_second_sc &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        read64(target_addr) == final_sc_payload &&
        read64(target_addr) != blocked_sc_payload &&
        read64(neighbor_addr) == neighbor_payload &&
        read64(marker_addr) == marker_payload &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI enabled only after lr.d, the target-address breakpoint skips sc.d and leaves the SC status register sentinel plus all memory words unchanged",
        blocked_sc_suppressed_store_intent);
    TEST_ASSERT("while the SC target trigger is still armed, unrelated store/load progress does not observe or replay the blocked SC payload",
        unrelated_store_load_does_not_observe_blocked_payload);
    TEST_ASSERT("after clearing the trigger and VModule source, the next SC result is self-consistent with target memory regardless of whether the platform preserved the reservation",
        second_sc_status_matches_memory);
    TEST_ASSERT("a fresh LR/SC pair recovers cleanly and no late blocked-SC payload, breakpoint, or MEI replay appears after the recovery store",
        fresh_lrsc_commits_after_store_intent_probe &&
        no_late_sc_payload_or_trap_replay);

    TEST_END(ai_interrupt_m_vmodule_lrsc_sc_blocked_no_stale_store_intent_corner);
}

bool ai_interrupt_m_vmodule_two_slot_chain_partial_match_no_cross_episode_leak_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t miss_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uintptr_t chain_store_pc =
        (uintptr_t)ai_ibc_chain_exec_store_store_site;
    const uint64_t guard_seed = 0x636861696e677001ULL;
    const uint64_t miss_seed = 0x636861696e6d7302ULL;
    const uint64_t target_seed = 0x636861696e746703ULL;
    const uint64_t after_seed = 0x636861696e616604ULL;
    const uint64_t partial_payload = 0x636861696e703110ULL;
    const uint64_t unrelated_payload = 0x636861696e753220ULL;
    const uint64_t blocked_payload = 0x636861696e623330ULL;
    const uint64_t committed_payload = 0x636861696e633440ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t slot0_tdata1 = 0ULL;
    uint64_t slot0_tdata2 = 0ULL;
    uint64_t slot1_tdata1 = 0ULL;
    uint64_t slot1_tdata2 = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(miss_addr, miss_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_arm_mcontrol6_slot(0, chain_store_pc,
        AI_IBC_MCONTROL6_EXECUTE, true);
    ai_ibc_arm_mcontrol6_slot(1, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_vmodule_meip_pending(false, 1ULL);
    ai_ibc_chain_exec_store_probe((uint64_t *)miss_addr, partial_payload);
    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    ai_ibc_read_trigger_csr_slot(0, &slot0_tdata1, &slot0_tdata2);
    ai_ibc_read_trigger_csr_slot(1, &slot1_tdata1, &slot1_tdata2);

    const bool partial_match_interrupt_no_breakpoint =
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        (slot0_tdata1 & AI_IBC_MCONTROL6_CHAIN) != 0ULL &&
        (slot0_tdata1 & AI_IBC_MCONTROL6_EXECUTE) != 0ULL &&
        (slot0_tdata1 & AI_IBC_MCONTROL6_STORE) == 0ULL &&
        slot0_tdata2 == chain_store_pc &&
        (slot1_tdata1 & AI_IBC_MCONTROL6_CHAIN) == 0ULL &&
        (slot1_tdata1 & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        (slot1_tdata1 & AI_IBC_MCONTROL6_EXECUTE) == 0ULL &&
        slot1_tdata2 == target_addr &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == partial_payload &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    const uint64_t trap_count_after_partial = ai_ibc_trap_count;
    ai_ibc_store_once(target_addr, unrelated_payload);
    const bool unrelated_address_probe_no_ghost =
        ai_ibc_trap_count == trap_count_after_partial &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == partial_payload &&
        read64(target_addr) == unrelated_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    ai_ibc_chain_exec_store_probe((uint64_t *)target_addr, blocked_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
            ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool closed_chain_breaks_without_losing_config =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, chain_store_pc) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 2ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == partial_payload &&
        read64(target_addr) == unrelated_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) != 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t trap_count_after_clear = ai_ibc_trap_count;
    ai_ibc_chain_exec_store_probe((uint64_t *)target_addr,
        committed_payload);
    const bool clear_trigger_allows_exactly_once_commit =
        ai_ibc_trap_count == trap_count_after_clear &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == partial_payload &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with VModule MEI in a two-slot chain window, an execute-PC first-slot match plus a nonmatching store address commits without breakpoint and clears the MEI source once",
        partial_match_interrupt_no_breakpoint);
    TEST_ASSERT("after that trap episode, an unrelated store PC that only matches the chained address slot does not inherit a stale first-slot match or raise a ghost breakpoint",
        unrelated_address_probe_no_ghost);
    TEST_ASSERT("when the same two-slot chain is truly closed by the execute-PC store site and target address, breakpoint is observed once and the store side effect is suppressed",
        closed_chain_breaks_without_losing_config);
    TEST_ASSERT("after clearing the chain and VModule source, the same store helper commits exactly once without new trap log entries",
        clear_trigger_allows_exactly_once_commit);

    TEST_END(ai_interrupt_m_vmodule_two_slot_chain_partial_match_no_cross_episode_leak_corner);
}

bool ai_interrupt_m_stip_pending_trigger_slot_rearm_no_ghost_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t miss_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[3];
    const uint64_t guard_seed = 0x1020304050607080ULL;
    const uint64_t miss_seed = 0x8877665544332211ULL;
    const uint64_t target_seed = 0x99aabbccddeeff00ULL;
    const uint64_t after_seed = 0x1234432112344321ULL;
    const uint64_t miss_payload0 = 0x0102030405060708ULL;
    const uint64_t miss_payload1 = 0x1112131415161718ULL;
    const uint64_t blocked_payload = 0x2122232425262728ULL;
    const uint64_t committed_payload = 0x3132333435363738ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(miss_addr, miss_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_clear_mcontrol6_slot(0);

    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(miss_addr, miss_payload0);
    const bool first_miss_no_ghost =
        excpt.triggered == false &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == miss_payload0 &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, committed_payload);
    const bool clear_allows_target_commit =
        excpt.triggered == false &&
        read64(target_addr) == committed_payload &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_arm_mcontrol6_slot(0, miss_addr, AI_IBC_MCONTROL6_STORE,
        false);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, blocked_payload);
    const bool rearm_to_miss_addr_no_ghost =
        excpt.triggered == false &&
        read64(miss_addr) == miss_payload0 &&
        read64(target_addr) == blocked_payload &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, miss_payload1);
    const bool final_rearm_breaks =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        read64(guard_addr) == guard_seed &&
        read64(miss_addr) == miss_payload0 &&
        read64(target_addr) == blocked_payload &&
        read64(after_addr) == after_seed &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_interrupt =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending disabled, a store trigger armed for another address does not create a ghost breakpoint and lets the miss store commit",
        first_miss_no_ghost);
    TEST_ASSERT("clearing the trigger slot in the same pending window lets the former target store commit normally",
        clear_allows_target_commit);
    TEST_ASSERT("rearming slot0 to a different store address still produces no ghost breakpoint on the old target and preserves the pending STIP",
        rearm_to_miss_addr_no_ghost);
    TEST_ASSERT("after rearming slot0 back to the target address, the next target store reports breakpoint and suppresses the side effect",
        final_rearm_breaks);
    TEST_ASSERT("after repeated trigger slot rewrites, the original pending STIP is still delivered exactly once when enabled",
        interrupt_seen &&
        pending_cleared_after_interrupt);

    TEST_END(ai_interrupt_m_stip_pending_trigger_slot_rearm_no_ghost_corner);
}

bool ai_interrupt_m_store_address_trigger_stip_retry_no_duplicate_uop_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t guard_seed = 0x7270647570676401ULL;
    const uint64_t target_seed = 0x7270647570746702ULL;
    const uint64_t after_seed = 0x7270647570616603ULL;
    const uint64_t blocked_payload = 0x72706475626c6b10ULL;
    const uint64_t committed_payload = 0x72706475636d7420ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t setup_mip = CSRR(CSR_MIP);
    ai_ibc_store_once(target_addr, blocked_payload);
    marker += 1ULL;
    const uint64_t trap_count_after_breakpoint = ai_ibc_trap_count;
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    uint64_t tdata1_after_breakpoint = 0ULL;
    uint64_t tdata2_after_breakpoint = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_breakpoint,
        &tdata2_after_breakpoint);

    const bool first_store_breaks_once =
        trap_count_after_breakpoint == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == store_pc &&
        ai_ibc_trap_tval[0] == target_addr &&
        ai_ibc_log_count_cause_epc_tval(CAUSE_BKP, store_pc,
            target_addr) == 1ULL &&
        (setup_mip & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        marker == 1ULL;
    const bool trigger_tuple_stays_current =
        ai_ibc_mcontrol6_type(tdata1_after_breakpoint) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        tdata2_after_breakpoint == target_addr;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_duplicate_breakpoint =
        ai_ibc_trap_count == trap_count_after_breakpoint &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);
    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    const bool stip_drained_without_replay =
        trap_count_after_interrupt == trap_count_after_breakpoint + 1ULL &&
        ai_ibc_trap_cause[trap_count_after_breakpoint] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[trap_count_after_breakpoint] &
            AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed &&
        marker == 2ULL;

    TEST_SETUP_EXCEPT();
    ai_ibc_store_once(target_addr, committed_payload);
    marker += 1ULL;
    const bool retry_store_commits_exactly_once =
        !excpt.triggered &&
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed &&
        marker == 3ULL;
    excpt.testing = false;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but locally disabled, the first target store reports one address-trigger breakpoint and suppresses its payload",
        first_store_breaks_once,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx setup_mip=%lx mip=%lx target=%lx marker=%lx",
        trap_count_after_breakpoint, ai_ibc_trap_cause[0],
        ai_ibc_trap_epc[0], ai_ibc_trap_tval[0], setup_mip,
        mip_after_breakpoint, read64(target_addr), marker);
    TEST_ASSERT("the address-trigger tuple remains bound to the target VA after the breakpoint and no late duplicate breakpoint appears while the same pending STIP is masked",
        trigger_tuple_stays_current &&
        no_late_duplicate_breakpoint,
        "tdata1=%lx tdata2=%lx count=%lx bkp=%lx mip=%lx target=%lx",
        tdata1_after_breakpoint, tdata2_after_breakpoint,
        ai_ibc_trap_count, ai_ibc_log_count_cause(CAUSE_BKP),
        CSRR(CSR_MIP), read64(target_addr));
    TEST_ASSERT("after clearing the trigger, the preserved STIP is delivered exactly once without replaying the trapped store",
        stip_drained_without_replay,
        "count=%lx cause1=%lx mip1=%lx mip_after=%lx target=%lx marker=%lx",
        trap_count_after_interrupt,
        ai_ibc_trap_cause[trap_count_after_breakpoint],
        ai_ibc_trap_mip[trap_count_after_breakpoint],
        mip_after_interrupt, read64(target_addr), marker);
    TEST_ASSERT("with trigger and STIP both drained, retrying the store commits the payload exactly once and leaves no ghost trap event",
        retry_store_commits_exactly_once);

    TEST_END(ai_interrupt_m_store_address_trigger_stip_retry_no_duplicate_uop_corner);
}

bool ai_interrupt_m_stip_handler_clears_trigger_slot_rearm_breakpoint_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x43544c52544c5201ULL;
    const uint64_t target_seed = 0x43544c52544c5202ULL;
    const uint64_t after_seed = 0x43544c52544c5203ULL;
    const uint64_t cleared_probe_payload = 0x43544c52544c5204ULL;
    const uint64_t blocked_payload = 0x43544c52544c5205ULL;
    const uint64_t committed_payload = 0x43544c52544c5206ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_prepare_stip_pending(true);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_CLEAR_SLOT0_ON_STI;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_interrupt_cleared_trigger_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool interrupt_recorded_and_cleared =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_interrupt_cleared_trigger_hits == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    const uint64_t trap_count_after_handler = ai_ibc_trap_count;
    const uint64_t breakpoint_count_after_handler =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, cleared_probe_payload);
    const bool handler_clear_allows_target_commit =
        ai_ibc_trap_count == trap_count_after_handler &&
        ai_ibc_log_count_cause(CAUSE_BKP) ==
            breakpoint_count_after_handler &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == cleared_probe_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    const uint64_t breakpoint_count_after_interrupt =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, blocked_payload);
    const bool rearm_after_interrupt_breaks =
        ai_ibc_trap_count == trap_count_after_interrupt + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) ==
            breakpoint_count_after_interrupt + 1ULL &&
        ai_ibc_trap_cause[trap_count_after_interrupt] == CAUSE_BKP &&
        ai_ibc_trap_epc[trap_count_after_interrupt] ==
            (uintptr_t)ai_ibc_store_once &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == cleared_probe_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t breakpoint_count_after_rearm =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, committed_payload);
    const bool clear_after_rearm_allows_commit =
        ai_ibc_log_count_cause(CAUSE_BKP) ==
            breakpoint_count_after_rearm &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the STIP handler records and clears the interrupt once while leaving target memory untouched",
        interrupt_recorded_and_cleared);
    TEST_ASSERT("the trigger slot cleared inside the interrupt handler no longer reports breakpoint before the testcase rearms it",
        handler_clear_allows_target_commit);
    TEST_ASSERT("after interrupt return, rearming the same slot0 store trigger produces a fresh breakpoint and suppresses the target store",
        rearm_after_interrupt_breaks);
    TEST_ASSERT("clearing slot0 after the fresh breakpoint removes the trigger so the control store commits normally",
        clear_after_rearm_allows_commit);

    TEST_END(ai_interrupt_m_stip_handler_clears_trigger_slot_rearm_breakpoint_corner);
}

bool ai_interrupt_m_breakpoint_handler_clears_stip_preserves_trigger_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x42504b5354495001ULL;
    const uint64_t target_seed = 0x42504b5354495002ULL;
    const uint64_t after_seed = 0x42504b5354495003ULL;
    const uint64_t first_blocked_payload = 0x42504b5354495004ULL;
    const uint64_t second_blocked_payload = 0x42504b5354495005ULL;
    const uint64_t committed_payload = 0x42504b5354495006ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_STORE,
        false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_CLEAR_STIP_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_ibc_store_once(target_addr, first_blocked_payload);
    const bool first_breakpoint_clears_only_stip =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_store_once &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    const uint64_t trap_count_after_first = ai_ibc_trap_count;
    const uint64_t breakpoint_count_after_first =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, second_blocked_payload);
    const bool trigger_preserved_after_pending_clear =
        ai_ibc_trap_count == trap_count_after_first + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) ==
            breakpoint_count_after_first + 1ULL &&
        ai_ibc_trap_cause[trap_count_after_first] == CAUSE_BKP &&
        ai_ibc_trap_epc[trap_count_after_first] ==
            (uintptr_t)ai_ibc_store_once &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    const uint64_t breakpoint_count_after_clear =
        ai_ibc_log_count_cause(CAUSE_BKP);
    ai_ibc_store_once(target_addr, committed_payload);
    const bool trigger_clear_allows_commit =
        ai_ibc_log_count_cause(CAUSE_BKP) ==
            breakpoint_count_after_clear &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == committed_payload &&
        read64(after_addr) == after_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the first store breakpoint handler clears the pending STIP while preserving the breakpoint-suppressed memory image",
        first_breakpoint_clears_only_stip);
    TEST_ASSERT("clearing interrupt pending in the breakpoint handler does not clear the still-armed store trigger; the same helper breaks again",
        trigger_preserved_after_pending_clear);
    TEST_ASSERT("only an explicit trigger-slot clear lets the control store commit after the repeated breakpoint episode",
        trigger_clear_allows_commit);

    TEST_END(ai_interrupt_m_breakpoint_handler_clears_stip_preserves_trigger_corner);
}

bool ai_interrupt_m_counter_csr_logging_breakpoint_stip_trap_state_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_COUNTER_LOG;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_ebreak_probe();
    marker += 1ULL;

    const bool breakpoint_log_with_counters =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_counter_cycle[0] != 0ULL &&
        ai_ibc_counter_instret[0] != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        marker == 1ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool interrupt_log_with_counters =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_counter_cycle[1] != 0ULL &&
        ai_ibc_counter_instret[1] != 0ULL &&
        ai_ibc_counter_cycle[1] >= ai_ibc_counter_cycle[0] &&
        ai_ibc_counter_instret[1] >= ai_ibc_counter_instret[0] &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 2ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("reading mcycle/minstret in the breakpoint handler does not corrupt the logged breakpoint cause, epc, pending STIP, or marker progress",
        breakpoint_log_with_counters);
    TEST_ASSERT("reading the same counter CSRs in the later STIP handler preserves the interrupt cause and clears only the pending STIP once",
        interrupt_log_with_counters);

    TEST_END(ai_interrupt_m_counter_csr_logging_breakpoint_stip_trap_state_corner);
}

bool ai_interrupt_m_nested_handler_log_scratch_preserved_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t main_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_stip_handler_ebreak_probe);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_nested_after_marker == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    main_marker = 0x6c6f6773ULL;

    const bool nested_slot_contract =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_magic[0] == AI_IBC_NESTED_LOG_MAGIC_BASE &&
        ai_ibc_trap_magic[1] == (AI_IBC_NESTED_LOG_MAGIC_BASE + 1ULL) &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] ==
            (uintptr_t)ai_ibc_nested_handler_ebreak_site;
    const bool outer_scratch_survived_nested_breakpoint =
        ai_ibc_nested_outer_epc != 0ULL &&
        ai_ibc_nested_outer_mstatus != 0ULL &&
        ai_ibc_trap_epc[0] == ai_ibc_nested_outer_epc &&
        ai_ibc_trap_mstatus[0] == ai_ibc_nested_outer_mstatus &&
        ai_ibc_nested_outer_epc !=
            (uintptr_t)ai_ibc_nested_handler_ebreak_site;
    const bool tail_and_pending_contract =
        ai_ibc_nested_after_marker == 0x73746962ULL &&
        main_marker == 0x6c6f6773ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the outer STIP handler slot and the nested breakpoint slot keep separate magic, cause, and epc records",
        nested_slot_contract);
    TEST_ASSERT("the outer handler scratch copy of mepc/mstatus survives the handler-local breakpoint and is not overwritten by the inner trap log",
        outer_scratch_survived_nested_breakpoint);
    TEST_ASSERT("after the nested breakpoint returns, the outer handler reaches its tail marker, clears STIP once, and returns to the main flow once",
        tail_and_pending_contract);

    TEST_END(ai_interrupt_m_nested_handler_log_scratch_preserved_corner);
}

bool ai_interrupt_m_illegal_breakpoint_pending_stip_priority_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t rvi_marker = 0ULL;
    volatile uint64_t rvc_marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SKIP_ILLEGAL;

    ai_ibc_ebreak_illegal32_probe(&rvi_marker);
    const bool rvi_breakpoint_then_illegal =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] ==
            (uintptr_t)ai_ibc_ebreak_illegal32_site &&
        ai_ibc_trap_cause[1] == CAUSE_ILLEGAL_INSTRUCTION &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_illegal32_site &&
        rvi_marker == 0x693332ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    ai_ibc_reset_log();
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SKIP_ILLEGAL;
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_ibc_ebreak_cillegal16_probe(&rvc_marker);
    const bool rvc_breakpoint_then_compressed_illegal =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] ==
            (uintptr_t)ai_ibc_ebreak_cillegal16_site &&
        ai_ibc_trap_cause[1] == CAUSE_ILLEGAL_INSTRUCTION &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_cillegal16_site &&
        rvc_marker == 0x693136ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool pending_interrupt_after_sync_exceptions =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[2] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_ILLEGAL_INSTRUCTION) == 1ULL &&
        rvi_marker == 0x693332ULL &&
        rvc_marker == 0x693136ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but disabled, the RVI ebreak is logged before the following 32-bit illegal instruction and the pending interrupt stays visible",
        rvi_breakpoint_then_illegal);
    TEST_ASSERT("the compressed illegal instruction immediately after an ebreak is skipped with 2-byte length and does not consume the pending STIP",
        rvc_breakpoint_then_compressed_illegal);
    TEST_ASSERT("only after both synchronous trap episodes does enabling STIE/MIE deliver the preserved STIP exactly once",
        pending_interrupt_after_sync_exceptions);

    TEST_END(ai_interrupt_m_illegal_breakpoint_pending_stip_priority_corner);
}

bool ai_interrupt_m_mcountinhibit_toggle_counter_oracle_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mcountinhibit = CSRR(CSR_MCOUNTINHIBIT);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    CSRC(CSR_MCOUNTINHIBIT, AI_IBC_MCOUNTINHIBIT_CY_IR);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_COUNTER_INHIBIT;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const bool breakpoint_counter_inhibit_contract =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_counter_cycle[0] != 0ULL &&
        ai_ibc_counter_instret[0] != 0ULL &&
        (ai_ibc_counter_inhibit_after[0] &
            AI_IBC_MCOUNTINHIBIT_CY_IR) ==
            AI_IBC_MCOUNTINHIBIT_CY_IR &&
        ai_ibc_counter_inhibit_restored[0] ==
            ai_ibc_counter_inhibit_before[0] &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        marker == 1ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;

    const bool interrupt_counter_inhibit_contract =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_counter_cycle[1] != 0ULL &&
        ai_ibc_counter_instret[1] != 0ULL &&
        (ai_ibc_counter_inhibit_after[1] &
            AI_IBC_MCOUNTINHIBIT_CY_IR) ==
            AI_IBC_MCOUNTINHIBIT_CY_IR &&
        ai_ibc_counter_inhibit_restored[1] ==
            ai_ibc_counter_inhibit_before[1] &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        marker == 2ULL;
    const bool final_counter_csr_restored =
        CSRR(CSR_MCOUNTINHIBIT) ==
            ai_ibc_counter_inhibit_before[1];

    CSRW(CSR_MCOUNTINHIBIT, saved_mcountinhibit);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("temporary mcountinhibit writes inside the breakpoint handler restore the CSR and preserve breakpoint cause, epc, pending STIP, and marker progress",
        breakpoint_counter_inhibit_contract);
    TEST_ASSERT("the later STIP handler can perform the same counter CSR instrumentation without corrupting interrupt cause or pending cleanup",
        interrupt_counter_inhibit_contract);
    TEST_ASSERT("after both handler episodes, mcountinhibit is back to the handler-entry value before testcase state restoration",
        final_counter_csr_restored);

    TEST_END(ai_interrupt_m_mcountinhibit_toggle_counter_oracle_corner);
}

bool ai_interrupt_m_cbozero_execute_trigger_stip_ordering_no_lost_zero_corner()
{
    TEST_START();

    const enum test_page target_page = VSRW_GRWX;
    const uintptr_t block_paddr = phys_page_base(target_page) + 0x700U;
    const uintptr_t before_guard_paddr = block_paddr - sizeof(uint64_t);
    const uintptr_t after_guard_paddr = block_paddr + 64U;
    const uintptr_t trigger_pc = (uintptr_t)ai_ibc_cbozero_site;
    const uint64_t before_guard_seed = 0x63626f7a67623001ULL;
    const uint64_t after_guard_seed = 0x63626f7a67613002ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t seed[8];
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (size_t i = 0; i < 8U; ++i) {
        seed[i] = 0x63626f7a626c3000ULL |
            (0x0101010101010101ULL * i);
        write64(block_paddr + i * sizeof(uint64_t), seed[i]);
    }
    write64(before_guard_paddr, before_guard_seed);
    write64(after_guard_paddr, after_guard_seed);
    asm volatile("fence rw, rw" ::: "memory");

    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_NONE;
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_arm_mcontrol6_slot(0, trigger_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    const uint64_t setup_mip = CSRR(CSR_MIP);
    ai_ibc_cbozero_once(block_paddr);
    marker += 1ULL;
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    uint64_t tdata1_after_breakpoint = 0ULL;
    uint64_t tdata2_after_breakpoint = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_breakpoint,
        &tdata2_after_breakpoint);

    bool block_unchanged_after_breakpoint = true;
    for (size_t i = 0; i < 8U; ++i) {
        block_unchanged_after_breakpoint =
            block_unchanged_after_breakpoint &&
            read64(block_paddr + i * sizeof(uint64_t)) == seed[i];
    }
    const bool breakpoint_blocks_cbozero_side_effect =
        (block_paddr & 63ULL) == 0ULL &&
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == trigger_pc &&
        (ai_ibc_trap_tval[0] == 0ULL ||
            ai_ibc_trap_tval[0] == trigger_pc) &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (setup_mip & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        block_unchanged_after_breakpoint &&
        read64(before_guard_paddr) == before_guard_seed &&
        read64(after_guard_paddr) == after_guard_seed &&
        marker == 1ULL;
    const bool trigger_tuple_survives_breakpoint =
        ai_ibc_mcontrol6_type(tdata1_after_breakpoint) ==
            AI_IBC_MCONTROL6_TYPE_MCONTROL6 &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_EXECUTE) != 0ULL &&
        (tdata1_after_breakpoint & AI_IBC_MCONTROL6_MSU) ==
            AI_IBC_MCONTROL6_MSU &&
        tdata2_after_breakpoint == trigger_pc;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);
    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;

    bool block_unchanged_after_interrupt = true;
    for (size_t i = 0; i < 8U; ++i) {
        block_unchanged_after_interrupt =
            block_unchanged_after_interrupt &&
            read64(block_paddr + i * sizeof(uint64_t)) == seed[i];
    }
    const bool stip_delivered_once_without_lost_zero =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        block_unchanged_after_interrupt &&
        read64(before_guard_paddr) == before_guard_seed &&
        read64(after_guard_paddr) == after_guard_seed &&
        marker == 2ULL;

    ai_ibc_cbozero_once(block_paddr);
    marker += 1ULL;
    bool block_zeroed_after_retry = true;
    for (size_t i = 0; i < 8U; ++i) {
        block_zeroed_after_retry =
            block_zeroed_after_retry &&
            read64(block_paddr + i * sizeof(uint64_t)) == 0ULL;
    }
    const bool clear_trigger_allows_ordered_cbozero =
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        block_zeroed_after_retry &&
        read64(before_guard_paddr) == before_guard_seed &&
        read64(after_guard_paddr) == after_guard_seed &&
        marker == 3ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but masked, an execute trigger on cbo.zero reports a breakpoint and leaves the whole target block plus both guards unchanged",
        breakpoint_blocks_cbozero_side_effect &&
        trigger_tuple_survives_breakpoint,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx mip_setup=%lx mip_after=%lx tdata1=%lx tdata2=%lx before=%lx after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_tval[0], setup_mip, mip_after_breakpoint,
        tdata1_after_breakpoint, tdata2_after_breakpoint,
        read64(before_guard_paddr), read64(after_guard_paddr), marker);
    TEST_ASSERT("after the trigger is cleared, the preserved STIP is delivered exactly once and does not create a delayed cbo.zero side effect",
        stip_delivered_once_without_lost_zero,
        "count=%lx cause1=%lx mip1=%lx mip_after=%lx before=%lx after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1], ai_ibc_trap_mip[1],
        mip_after_interrupt, read64(before_guard_paddr),
        read64(after_guard_paddr), marker);
    TEST_ASSERT("a retry without the execute trigger zeroes exactly the target CBO block once and preserves adjacent cacheable DRAM guards",
        clear_trigger_allows_ordered_cbozero,
        "count=%lx bkp=%lx sti=%lx first=%lx last=%lx before=%lx after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_log_count_cause(CAUSE_BKP),
        ai_ibc_log_count_cause(CAUSE_STI), read64(block_paddr),
        read64(block_paddr + 7U * sizeof(uint64_t)),
        read64(before_guard_paddr), read64(after_guard_paddr), marker);

    TEST_END(ai_interrupt_m_cbozero_execute_trigger_stip_ordering_no_lost_zero_corner);
}

bool ai_interrupt_hs_fetch_page_fault_execute_trigger_stip_no_stale_block_corner()
{
    TEST_START();

    typedef uint64_t (*ai_ibc_fetch_func_t)(void);

    const enum test_page exec_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRW_GRWX;
    const uintptr_t offset = 0x880U;
    const uintptr_t exec_vaddr = hs_page_base(exec_page) + offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + offset;
    const uintptr_t exec_paddr = phys_page_base(exec_page) + offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + offset;
    const uintptr_t trigger_pc = (uintptr_t)ai_ibc_prefetch_i_once;
    const uint64_t exec_code = 0x0000806702100513ULL;
    const uint64_t fault_code = 0x0000806703d00513ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    uint64_t exec_break_cause = 0ULL;
    uint64_t exec_break_priv = 0ULL;
    uint64_t exec_break_epc = 0ULL;
    uint64_t exec_break_tval = 0ULL;
    uint64_t no_x_fault_cause = 0ULL;
    uint64_t no_x_fault_priv = 0ULL;
    uint64_t no_x_fault_epc = 0ULL;
    uint64_t no_x_fault_tval = 0ULL;
    ai_ibc_fetch_func_t exec_func = (ai_ibc_fetch_func_t)exec_vaddr;
    pte_t *exec_pte = ai_ibc_hs_leaf_entry(exec_page);
    pte_t *fault_pte = ai_ibc_hs_leaf_entry(fault_page);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(exec_paddr, exec_code);
    write64(fault_paddr, fault_code);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MEDELEG, (1ULL << CAUSE_BKP) | (1ULL << CAUSE_IPF));
    ai_ibc_prepare_stip_pending(false);
    hspt_init();
    *exec_pte = (phys_page_base(exec_page) >> 2) |
        PTE_V | PTE_AD | PTE_RWX;
    *fault_pte = (phys_page_base(fault_page) >> 2) |
        PTE_V | PTE_AD | PTE_RW;
    sfence_vma();
    ai_ibc_fence_i();
    ai_ibc_arm_mcontrol6_slot_priv(0, trigger_pc,
        AI_IBC_MCONTROL6_EXECUTE, AI_IBC_MCONTROL6_MSU, false);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | MSTATUS_SIE);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_ibc_prefetch_i_once(exec_vaddr);
    marker += 1ULL;
    exec_break_cause = excpt.cause;
    exec_break_priv = excpt.priv;
    exec_break_epc = excpt.epc;
    exec_break_tval = excpt.tval;
    goto_priv(PRIV_M);
    const uint64_t pending_after_exec_breakpoint = CSRR(CSR_MIP);
    const bool executable_block_breakpoint_current =
        exec_break_priv == PRIV_HS &&
        exec_break_cause == CAUSE_BKP &&
        exec_break_epc == trigger_pc &&
        exec_break_tval != fault_vaddr &&
        (pending_after_exec_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(exec_paddr) == exec_code &&
        read64(fault_paddr) == fault_code &&
        marker == 1ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(fault_vaddr);
    marker += 1ULL;
    no_x_fault_cause = excpt.cause;
    no_x_fault_priv = excpt.priv;
    no_x_fault_epc = excpt.epc;
    no_x_fault_tval = excpt.tval;
    goto_priv(PRIV_M);
    const uint64_t pending_after_fetch_fault = CSRR(CSR_MIP);
    const bool no_x_block_reports_fresh_ipf_not_old_breakpoint =
        no_x_fault_priv == PRIV_HS &&
        no_x_fault_cause == CAUSE_IPF &&
        no_x_fault_epc == fault_vaddr &&
        no_x_fault_tval == fault_vaddr &&
        no_x_fault_epc != exec_vaddr &&
        (pending_after_fetch_fault & AI_IBC_MIP_STIP) != 0ULL &&
        read64(exec_paddr) == exec_code &&
        read64(fault_paddr) == fault_code &&
        marker == 2ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    marker += 1ULL;
    const uint64_t pending_after_interrupt = CSRR(CSR_MIP);

    TEST_SETUP_EXCEPT();
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool no_late_repeat_stip =
        !excpt.triggered &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;
    excpt.testing = false;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t final_exec_value = exec_func();
    marker += 1ULL;
    const bool trigger_clear_recovers_executable_block =
        !excpt.triggered &&
        final_exec_value == 0x21ULL &&
        read64(exec_paddr) == exec_code &&
        read64(fault_paddr) == fault_code &&
        marker == 4ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("with STIP pending but disabled, the HS fetch probe hits the armed execute trigger at the current helper PC instead of inheriting the later no-X VA",
        executable_block_breakpoint_current,
        "cause=%lx priv=%lx epc=%lx tval=%lx trigger=%lx mip=%lx exec=%lx fault=%lx marker=%lx",
        exec_break_cause, exec_break_priv, exec_break_epc,
        exec_break_tval, trigger_pc,
        pending_after_exec_breakpoint, read64(exec_paddr),
        read64(fault_paddr), marker);
    TEST_ASSERT("while the old execute trigger remains armed on block A, the distinct no-X block B reports instruction page fault with its own VA and keeps STIP pending",
        no_x_block_reports_fresh_ipf_not_old_breakpoint,
        "cause=%lx priv=%lx epc=%lx tval=%lx mip=%lx exec_va=%lx fault_va=%lx marker=%lx",
        no_x_fault_cause, no_x_fault_priv, no_x_fault_epc,
        no_x_fault_tval,
        pending_after_fetch_fault, exec_vaddr, fault_vaddr, marker);
    TEST_ASSERT("after clearing the trigger, the preserved STIP is delivered once and does not repeat before the executable block is retried",
        interrupt_seen &&
        (pending_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        no_late_repeat_stip);
    TEST_ASSERT("with the trigger cleared and STIP drained, the original executable block still fetches the programmed return image exactly once",
        trigger_clear_recovers_executable_block);

    TEST_END(ai_interrupt_hs_fetch_page_fault_execute_trigger_stip_no_stale_block_corner);
}

bool ai_interrupt_m_rvc_rvi_mixed_width_pending_stip_pc_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_ENABLE_STIP_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);

    ai_ibc_rvc_rvi_mixed_width_probe(&marker);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool mixed_width_event_order =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_mixed_cebreak_site &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_trap_cause[2] == CAUSE_BKP &&
        ai_ibc_trap_epc[2] == (uintptr_t)ai_ibc_mixed_ebreak_site;
    const bool marker_exactly_once =
        marker == 0x33ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL;
    const bool pending_and_pc_contract =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_epc[0] != ai_ibc_trap_epc[2]);

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the mixed RVC/RVI sequence reports c.ebreak first, then the pending STIP, then the later 32-bit ebreak without swapping epc records",
        mixed_width_event_order);
    TEST_ASSERT("both marker updates after the compressed and 32-bit breakpoints execute exactly once, proving skip length did not repeat or skip adjacent instructions",
        marker_exactly_once);
    TEST_ASSERT("the STIP is visible to the interrupt log and cleared once before the later breakpoint PC is recorded as a distinct address",
        pending_and_pc_contract);

    TEST_END(ai_interrupt_m_rvc_rvi_mixed_width_pending_stip_pc_corner);
}

bool ai_interrupt_m_back_to_back_trap_log_no_stale_record_corner()
{
    TEST_START();

    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t after_addr = (uintptr_t)&ai_ibc_mem[2];
    const uint64_t guard_seed = 0x425442544c4f4701ULL;
    const uint64_t target_seed = 0x425442544c4f4702ULL;
    const uint64_t after_seed = 0x425442544c4f4703ULL;
    const uint64_t phase_a_tval_poison0 = 0xaaaabbbbcccc0010ULL;
    const uint64_t phase_a_tval_poison1 = 0xaaaabbbbcccc0020ULL;
    const uint64_t phase_b_tval_poison0 = 0xddddeeeeffff0010ULL;
    const uint64_t phase_b_tval_poison1 = 0xddddeeeeffff0020ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(guard_addr, guard_seed);
    write64(target_addr, target_seed);
    write64(after_addr, after_seed);
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_reset_log();
    ai_ibc_trap_tval[0] = phase_a_tval_poison0;
    ai_ibc_trap_tval[1] = phase_a_tval_poison1;
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_LOAD,
        false);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_ENABLE_STIP_ON_BKP;
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    (void)ai_ibc_load_once(target_addr);
    marker += 0x1ULL;
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 0x2ULL;

    const bool breakpoint_then_interrupt_records =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_load_once &&
        ai_ibc_trap_tval[0] == target_addr &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        ai_ibc_trap_tval[0] != phase_a_tval_poison0 &&
        ai_ibc_trap_tval[1] != phase_a_tval_poison1 &&
        ai_ibc_trap_tval[1] != target_addr &&
        ai_ibc_trap_epc[0] != ai_ibc_trap_epc[1];
    ai_ibc_clear_mcontrol6_slot(0);
    const bool phase_a_return_contract =
        marker == 0x3ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    TEST_ASSERT("when a load-address breakpoint handler immediately opens a pending STIP, the two trap log slots keep independent cause, epc, mip, and fresh tval snapshots",
        breakpoint_then_interrupt_records,
        "count=%lx cause0=%lx cause1=%lx epc0=%lx epc1=%lx tval0=%lx tval1=%lx mip0=%lx mip1=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_epc[0], ai_ibc_trap_epc[1],
        ai_ibc_trap_tval[0], ai_ibc_trap_tval[1],
        ai_ibc_trap_mip[0], ai_ibc_trap_mip[1]);
    TEST_ASSERT("the breakpoint-to-interrupt episode returns through both handlers exactly once, clears the preserved STIP, and leaves load target memory untouched",
        phase_a_return_contract,
        "marker=%lx mip=%lx guard=%lx target=%lx after=%lx",
        marker, CSRR(CSR_MIP), read64(guard_addr), read64(target_addr),
        read64(after_addr));

    ai_ibc_reset_log();
    ai_ibc_trap_tval[0] = phase_b_tval_poison0;
    ai_ibc_trap_tval[1] = phase_b_tval_poison1;
    ai_ibc_prepare_stip_pending(true);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 0x4ULL;
    ai_ibc_arm_mcontrol6_slot(0, target_addr, AI_IBC_MCONTROL6_LOAD,
        false);
    (void)ai_ibc_load_once(target_addr);
    marker += 0x8ULL;

    const bool interrupt_then_breakpoint_records =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[0] == CAUSE_STI &&
        ai_ibc_trap_cause[1] == CAUSE_BKP &&
        ai_ibc_trap_epc[1] == (uintptr_t)ai_ibc_load_once &&
        ai_ibc_trap_tval[1] == target_addr &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_trap_epc[0] != ai_ibc_trap_epc[1] &&
        ai_ibc_trap_tval[0] != phase_b_tval_poison0 &&
        ai_ibc_trap_tval[0] != target_addr &&
        ai_ibc_trap_tval[1] != phase_b_tval_poison1 &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;
    ai_ibc_clear_mcontrol6_slot(0);
    const bool phase_b_return_contract =
        marker == 0xfULL &&
        read64(guard_addr) == guard_seed &&
        read64(target_addr) == target_seed &&
        read64(after_addr) == after_seed;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("when a STIP is immediately followed by a load-address breakpoint, the later breakpoint does not overwrite or reuse the interrupt slot record",
        interrupt_then_breakpoint_records,
        "count=%lx cause0=%lx cause1=%lx epc0=%lx epc1=%lx tval0=%lx tval1=%lx mip0=%lx mip1=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_cause[1],
        ai_ibc_trap_epc[0], ai_ibc_trap_epc[1],
        ai_ibc_trap_tval[0], ai_ibc_trap_tval[1],
        ai_ibc_trap_mip[0], ai_ibc_trap_mip[1]);
    TEST_ASSERT("the interrupt-to-breakpoint episode preserves marker progress through both adjacent trap returns and leaves the load target image unchanged",
        phase_b_return_contract,
        "marker=%lx guard=%lx target=%lx after=%lx",
        marker, read64(guard_addr), read64(target_addr),
        read64(after_addr));

    TEST_END(ai_interrupt_m_back_to_back_trap_log_no_stale_record_corner);
}

bool ai_interrupt_m_csrrs_csrrc_mie_window_ebreak_old_value_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_prepare_stip_pending(false);

    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mie_before_set = CSRR(CSR_MIE);
    const uint64_t old_for_set = ai_ibc_csrrs_mie_stie();
    const uint64_t mie_after_set = CSRR(CSR_MIE);

    const bool csrrs_old_value_and_pending =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe &&
        marker == 1ULL &&
        (mie_before_set & AI_IBC_MIE_STIE) == 0ULL &&
        old_for_set == mie_before_set &&
        (mie_after_set & AI_IBC_MIE_STIE) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    TEST_SETUP_EXCEPT();
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mie_before_clear = CSRR(CSR_MIE);
    const uint64_t old_for_clear = ai_ibc_csrrc_mie_stie();
    const uint64_t mie_after_clear = CSRR(CSR_MIE);

    const bool csrrc_old_value_and_pending =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.epc == (uintptr_t)ai_ibc_ebreak_probe &&
        marker == 2ULL &&
        (mie_before_clear & AI_IBC_MIE_STIE) != 0ULL &&
        old_for_clear == mie_before_clear &&
        (mie_after_clear & AI_IBC_MIE_STIE) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MIE) == 0ULL;

    const bool interrupt_not_opened_by_rmw_window =
        ai_ibc_enable_stip_and_wait();
    const bool pending_cleared_after_explicit_enable =
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL;

    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("after an ebreak in the mie RMW window, CSRRS returns the old mie value, sets only STIE, and keeps pending STIP behind global MIE",
        csrrs_old_value_and_pending);
    TEST_ASSERT("a second breakpoint before CSRRC does not corrupt the old-value readback or the final STIE clear",
        csrrc_old_value_and_pending);
    TEST_ASSERT("the pending STIP is delivered only after the testcase explicitly enables STIE/MIE after both CSR RMW windows",
        interrupt_not_opened_by_rmw_window &&
        pending_cleared_after_explicit_enable);

    TEST_END(ai_interrupt_m_csrrs_csrrc_mie_window_ebreak_old_value_corner);
}

bool ai_interrupt_hs_satp_switch_ebreak_stip_sfence_new_context_corner()
{
    TEST_START();

    const uintptr_t alias_vaddr = hs_page_base(SWITCH1) + 0x3c0U;
    const uintptr_t old_paddr = phys_page_base(SWITCH1) + 0x3c0U;
    const uintptr_t new_paddr = phys_page_base(SWITCH2) + 0x3c0U;
    const uintptr_t guard_old_paddr = old_paddr + sizeof(uint64_t);
    const uintptr_t guard_new_paddr = new_paddr + sizeof(uint64_t);
    const uint64_t old_seed = 0x736174706f6c6401ULL;
    const uint64_t new_seed = 0x736174706e657702ULL;
    const uint64_t old_guard_seed = 0x736174706f676403ULL;
    const uint64_t new_guard_seed = 0x736174706e676404ULL;
    const uint64_t new_commit_payload = 0x73617470636d7406ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t satp_old = ai_ibc_satp_for_root(ai_ibc_satp_root_old);
    const uint64_t satp_new = ai_ibc_satp_for_root(ai_ibc_satp_root_new);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(guard_old_paddr, old_guard_seed);
    write64(guard_new_paddr, new_guard_seed);
    ai_ibc_install_satp_root(ai_ibc_satp_root_old, ai_ibc_satp_l1_old,
        ai_ibc_satp_l2_old, ai_ibc_satp_l3_old, alias_vaddr, old_paddr,
        leaf_flags);
    ai_ibc_install_satp_root(ai_ibc_satp_root_new, ai_ibc_satp_l1_new,
        ai_ibc_satp_l2_new, ai_ibc_satp_l3_new, alias_vaddr, new_paddr,
        leaf_flags);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_STIE);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_prepare_stip_pending(false);
    ai_ibc_arm_mcontrol6_slot_priv(0, (uintptr_t)ai_ibc_ebreak_probe,
        AI_IBC_MCONTROL6_EXECUTE, AI_IBC_MCONTROL6_MSU, false);

    CSRW(CSR_SATP, satp_old);
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t old_context_value = ai_ibc_load_once(alias_vaddr);
    const bool old_context_visible =
        excpt.triggered == false &&
        old_context_value == old_seed &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_seed &&
        read64(guard_old_paddr) == old_guard_seed &&
        read64(guard_new_paddr) == new_guard_seed;
    excpt.testing = false;

    goto_priv(PRIV_M);
    CSRW(CSR_SATP, satp_new);
    const uint64_t satp_after_switch = CSRR(CSR_SATP);
    const uint64_t mip_before_breakpoint = CSRR(CSR_MIP);

    goto_priv(PRIV_HS);
    ai_ibc_ebreak_probe();
    marker += 1ULL;
    goto_priv(PRIV_M);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const bool breakpoint_in_satp_switch_window =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        satp_after_switch == satp_new &&
        (mip_before_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_seed &&
        marker == 1ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    sfence_vma();
    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t new_context_value = ai_ibc_load_once(alias_vaddr);
    ai_ibc_store_once(alias_vaddr, new_commit_payload);
    const bool sfence_access_no_exception = excpt.triggered == false;
    excpt.testing = false;
    marker += 1ULL;
    goto_priv(PRIV_M);
    const bool sfence_selects_new_context_only =
        sfence_access_no_exception &&
        new_context_value == new_seed &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_commit_payload &&
        read64(guard_old_paddr) == old_guard_seed &&
        read64(guard_new_paddr) == new_guard_seed &&
        ai_ibc_trap_count == 1ULL &&
        marker == 2ULL;

    const bool interrupt_seen = ai_ibc_enable_stip_and_wait();
    marker += 1ULL;
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    const uint64_t new_context_after_interrupt = ai_ibc_load_once(alias_vaddr);
    const bool post_interrupt_load_no_exception = excpt.triggered == false;
    excpt.testing = false;
    goto_priv(PRIV_M);
    const bool interrupt_does_not_revive_old_context =
        interrupt_seen &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IBC_MIP_STIP) == 0ULL &&
        post_interrupt_load_no_exception &&
        new_context_after_interrupt == new_commit_payload &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_commit_payload &&
        marker == 3ULL;

    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the old satp root maps the shared VA to the old physical marker before the switch window begins",
        old_context_visible,
        "old_value=%lx old_pa=%lx new_pa=%lx old_guard=%lx new_guard=%lx",
        old_context_value, read64(old_paddr), read64(new_paddr),
        read64(guard_old_paddr), read64(guard_new_paddr));
    TEST_ASSERT("after writing the new satp root but before sfence.vma, the ebreak breakpoint is precise and the pending STIP remains held behind disabled STIE/MIE",
        breakpoint_in_satp_switch_window,
        "count=%lx cause0=%lx epc0=%lx satp=%lx mip_before=%lx mip0=%lx mip_after=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        satp_after_switch, mip_before_breakpoint, ai_ibc_trap_mip[0],
        mip_after_breakpoint, marker);
    TEST_ASSERT("after sfence.vma, the same VA resolves only through the new satp root and the store updates the new physical marker without touching old root data",
        sfence_selects_new_context_only,
        "new_value=%lx old_pa=%lx new_pa=%lx old_guard=%lx new_guard=%lx marker=%lx",
        new_context_value, read64(old_paddr), read64(new_paddr),
        read64(guard_old_paddr), read64(guard_new_paddr), marker);
    TEST_ASSERT("delivering the preserved STIP after the satp switch does not resurrect the old translation or replay the breakpoint",
        interrupt_does_not_revive_old_context,
        "count=%lx cause1=%lx mip1=%lx mip_after=%lx value=%lx old_pa=%lx new_pa=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1], ai_ibc_trap_mip[1],
        mip_after_interrupt, new_context_after_interrupt,
        read64(old_paddr), read64(new_paddr), marker);

    TEST_END(ai_interrupt_hs_satp_switch_ebreak_stip_sfence_new_context_corner);
}

bool ai_interrupt_m_breakpoint_handler_repairs_pmp_stip_sees_final_permission_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x340U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x340U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t initial = 0x627072706d706901ULL;
    const uint64_t adjacent = 0x627072706d706a02ULL;
    const uint64_t denied_payload = 0x627072706d706403ULL;
    const uint64_t retry_payload = 0x627072706d707204ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    const uint64_t deny_cfg =
        ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_NO_W);
    const uint64_t repaired_cfg =
        ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_RWX);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    ai_ibc_restore_pmp_full_access();
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_ibc_prepare_mprv_user_translation();
    ai_ibc_configure_pmp_window(pmp_start, pmp_end,
        AI_IBC_PMP_TOR_NO_W);

    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t pre_repair_fault_cause = excpt.cause;
    const uint64_t pre_repair_fault_tval = excpt.tval;
    const uint64_t pre_repair_fault_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t pre_repair_target_after = read64(paddr);
    const uint64_t pre_repair_adjacent_after = read64(adjacent_paddr);
    const bool pre_repair_pmp_fault_condition =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        pre_repair_fault_cause == CAUSE_SAF &&
        pre_repair_fault_tval == vaddr &&
        pre_repair_fault_cfg == deny_cfg &&
        pre_repair_target_after == initial &&
        pre_repair_adjacent_after == adjacent;

    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_pmp_repair_addr0 = pmp_start >> 2U;
    ai_ibc_pmp_repair_addr1 = pmp_end >> 2U;
    ai_ibc_pmp_repair_addr2 = (uint64_t)-1;
    ai_ibc_pmp_repair_cfg = repaired_cfg;
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_REPAIR_PMP_ON_BKP;

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t pmpcfg_after_breakpoint = CSRR(CSR_PMPCFG0);
    const uint64_t pmpaddr0_after_breakpoint = CSRR(CSR_PMPADDR0);
    const uint64_t pmpaddr1_after_breakpoint = CSRR(CSR_PMPADDR1);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const bool breakpoint_repairs_full_pmp_before_interrupt =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_pmpcfg0[0] == deny_cfg &&
        ai_ibc_trap_pmpaddr0[0] == (pmp_start >> 2U) &&
        ai_ibc_trap_pmpaddr1[0] == (pmp_end >> 2U) &&
        ai_ibc_pmp_repair_hits == 1ULL &&
        pmpcfg_after_breakpoint == repaired_cfg &&
        pmpaddr0_after_breakpoint == (pmp_start >> 2U) &&
        pmpaddr1_after_breakpoint == (pmp_end >> 2U) &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        read64(paddr) == initial &&
        marker == 1ULL;

    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;
    const bool interrupt_sees_repaired_pmp_not_half_state =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        ai_ibc_trap_pmpcfg0[1] == repaired_cfg &&
        ai_ibc_trap_pmpaddr0[1] == (pmp_start >> 2U) &&
        ai_ibc_trap_pmpaddr1[1] == (pmp_end >> 2U) &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(paddr) == initial &&
        read64(adjacent_paddr) == adjacent &&
        marker == 2ULL;

    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, retry_payload);
    ai_ibc_clear_mprv_translation();
    const bool retry_after_repair_commits_once =
        excpt.triggered == false &&
        CSRR(CSR_PMPCFG0) == repaired_cfg &&
        read64(paddr) == retry_payload &&
        read64(adjacent_paddr) == adjacent &&
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, saved_pmpaddr0);
    CSRW(CSR_PMPADDR1, saved_pmpaddr1);
    CSRW(CSR_PMPADDR2, saved_pmpaddr2);
    CSRW(CSR_PMPCFG0, saved_pmpcfg0);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("before the repair breakpoint, the selected MPRV=U translated store really hits the PMP deny-write condition and leaves target/adjacent data unchanged",
        pre_repair_pmp_fault_condition,
        "cause=%lx tval=%lx cfg=%lx target=%lx adjacent=%lx",
        pre_repair_fault_cause, pre_repair_fault_tval,
        pre_repair_fault_cfg, pre_repair_target_after,
        pre_repair_adjacent_after);
    TEST_ASSERT("the breakpoint handler observes the denied PMP generation, repairs the full TOR window to RWX, and keeps the pending STIP behind disabled STIE/MIE",
        breakpoint_repairs_full_pmp_before_interrupt,
        "count=%lx cause0=%lx cfg0=%lx addr0=%lx addr1=%lx cfg_after=%lx mip=%lx hits=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0],
        ai_ibc_trap_pmpcfg0[0], ai_ibc_trap_pmpaddr0[0],
        ai_ibc_trap_pmpaddr1[0], pmpcfg_after_breakpoint,
        mip_after_breakpoint, ai_ibc_pmp_repair_hits, marker);
    TEST_ASSERT("after STIE/MIE opens, the pending STIP handler logs the repaired PMP cfg/address pair, not a half-repaired deny/off state",
        interrupt_sees_repaired_pmp_not_half_state,
        "count=%lx cause1=%lx cfg1=%lx addr0=%lx addr1=%lx mip=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1],
        ai_ibc_trap_pmpcfg0[1], ai_ibc_trap_pmpaddr0[1],
        ai_ibc_trap_pmpaddr1[1], CSRR(CSR_MIP), marker);
    TEST_ASSERT("the retry store after breakpoint repair and pending interrupt handling commits exactly once through the repaired PMP permission",
        retry_after_repair_commits_once,
        "target=%lx adjacent=%lx cfg=%lx count=%lx",
        read64(paddr), read64(adjacent_paddr), repaired_cfg,
        ai_ibc_trap_count);

    TEST_END(ai_interrupt_m_breakpoint_handler_repairs_pmp_stip_sees_final_permission_corner);
}

bool ai_interrupt_m_pmp_update_window_ebreak_stip_final_permission_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x2c0U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x2c0U;
    const uintptr_t adjacent_paddr = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t initial = 0x706d707570647401ULL;
    const uint64_t adjacent = 0x706d7061646a7402ULL;
    const uint64_t baseline_payload = 0x706d70616c6c6f03ULL;
    const uint64_t denied_payload = 0x706d7064656e7904ULL;
    const uint64_t allowed_payload = 0x706d70616c6f7705ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRC(CSR_MIDELEG, AI_IBC_MIP_STIP);
    ai_ibc_restore_pmp_full_access();
    write64(paddr, initial);
    write64(adjacent_paddr, adjacent);
    ai_ibc_prepare_mprv_user_translation();

    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, baseline_payload);
    ai_ibc_clear_mprv_translation();
    const bool baseline_mprv_store_allowed =
        excpt.triggered == false &&
        read64(paddr) == baseline_payload &&
        read64(adjacent_paddr) == adjacent;

    ai_ibc_reset_log();
    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, pmp_start >> 2U);
    CSRW(CSR_PMPADDR1, pmp_end >> 2U);
    const uint64_t deny_half_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t deny_half_addr0 = CSRR(CSR_PMPADDR0);
    const uint64_t deny_half_addr1 = CSRR(CSR_PMPADDR1);

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mip_after_deny_breakpoint = CSRR(CSR_MIP);
    const bool deny_window_breakpoint_precise =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL &&
        (ai_ibc_trap_mip[0] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_deny_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        deny_half_cfg == 0ULL &&
        deny_half_addr0 == (pmp_start >> 2U) &&
        deny_half_addr1 == (pmp_end >> 2U) &&
        marker == 1ULL;

    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_NO_W));
    const uint64_t deny_final_cfg = CSRR(CSR_PMPCFG0);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t final_deny_fault_cause = excpt.cause;
    const uint64_t final_deny_fault_tval = excpt.tval;
    const uint64_t final_deny_target_after = read64(paddr);
    const uint64_t final_deny_adjacent_after = read64(adjacent_paddr);
    const bool final_deny_blocks_store =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        final_deny_fault_cause == CAUSE_SAF &&
        final_deny_fault_tval == vaddr &&
        deny_final_cfg == ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_NO_W) &&
        final_deny_target_after == baseline_payload &&
        final_deny_adjacent_after == adjacent;

    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;
    const bool deny_phase_interrupt_once =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[1] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(paddr) == baseline_payload &&
        marker == 2ULL;

    ai_ibc_prepare_stip_pending(false);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, pmp_start >> 2U);
    CSRW(CSR_PMPADDR1, pmp_end >> 2U);
    const uint64_t allow_half_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t allow_half_addr0 = CSRR(CSR_PMPADDR0);
    const uint64_t allow_half_addr1 = CSRR(CSR_PMPADDR1);

    ai_ibc_ebreak_probe();
    marker += 1ULL;
    const uint64_t mip_after_allow_breakpoint = CSRR(CSR_MIP);
    const bool allow_window_breakpoint_precise =
        ai_ibc_trap_count == 3ULL &&
        ai_ibc_trap_cause[2] == CAUSE_BKP &&
        ai_ibc_trap_epc[2] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 1ULL &&
        (ai_ibc_trap_mip[2] & AI_IBC_MIP_STIP) != 0ULL &&
        (mip_after_allow_breakpoint & AI_IBC_MIP_STIP) != 0ULL &&
        allow_half_cfg == 0ULL &&
        allow_half_addr0 == (pmp_start >> 2U) &&
        allow_half_addr1 == (pmp_end >> 2U) &&
        marker == 3ULL;

    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_RWX));
    const uint64_t allow_final_cfg = CSRR(CSR_PMPCFG0);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allowed_payload);
    ai_ibc_clear_mprv_translation();
    const bool final_allow_store_commits =
        excpt.triggered == false &&
        allow_final_cfg == ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_RWX) &&
        read64(paddr) == allowed_payload &&
        read64(adjacent_paddr) == adjacent;

    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IBC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_STI) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    marker += 1ULL;
    const bool allow_phase_interrupt_once =
        ai_ibc_trap_count == 4ULL &&
        ai_ibc_trap_cause[3] == CAUSE_STI &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_STI) == 2ULL &&
        (ai_ibc_trap_mip[3] & AI_IBC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IBC_MIP_STIP) == 0ULL &&
        read64(paddr) == allowed_payload &&
        read64(adjacent_paddr) == adjacent &&
        marker == 4ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IBC_MIP_STIP);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, saved_pmpaddr0);
    CSRW(CSR_PMPADDR1, saved_pmpaddr1);
    CSRW(CSR_PMPADDR2, saved_pmpaddr2);
    CSRW(CSR_PMPCFG0, saved_pmpcfg0);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("baseline MPRV=U access proves the selected 4KB translated page is reachable before the PMP update window",
        baseline_mprv_store_allowed);
    TEST_ASSERT("with STIP pending but disabled, the breakpoint in the half-written deny update observes only pmpaddr staging and does not deliver the interrupt early",
        deny_window_breakpoint_precise,
        "count=%lx cause0=%lx mip0=%lx mip_after=%lx cfg=%lx addr0=%lx addr1=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_mip[0],
        mip_after_deny_breakpoint, deny_half_cfg, deny_half_addr0,
        deny_half_addr1, marker);
    TEST_ASSERT("after the final PMP deny cfg is written, the same translated store faults and leaves target and adjacent backing words unchanged",
        final_deny_blocks_store,
        "cause=%lx tval=%lx cfg=%lx target=%lx adjacent=%lx",
        final_deny_fault_cause, final_deny_fault_tval, deny_final_cfg,
        final_deny_target_after, final_deny_adjacent_after);
    TEST_ASSERT("the preserved STIP from the deny-update window is delivered exactly once after STIE/MIE opens and does not alter the final PMP-denied memory image",
        deny_phase_interrupt_once);
    TEST_ASSERT("a second breakpoint in the half-written allow update again sees the staged pmpaddr values while the new STIP remains pending behind disabled STIE/MIE",
        allow_window_breakpoint_precise,
        "count=%lx cause2=%lx mip2=%lx mip_after=%lx cfg=%lx addr0=%lx addr1=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[2], ai_ibc_trap_mip[2],
        mip_after_allow_breakpoint, allow_half_cfg, allow_half_addr0,
        allow_half_addr1, marker);
    TEST_ASSERT("after the final PMP allow cfg is written, the translated store commits exactly once, proving the earlier deny state was not permanently retained",
        final_allow_store_commits);
    TEST_ASSERT("the preserved STIP from the allow-update window is also delivered exactly once without changing the final allowed store result",
        allow_phase_interrupt_once);

    TEST_END(ai_interrupt_m_pmp_update_window_ebreak_stip_final_permission_corner);
}

bool ai_interrupt_hs_sfence_breakpoint_handler_stip_no_stale_trigger_va_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const enum test_page alias_page = SWITCH2;
    const uintptr_t target_va = hs_page_base(target_page) + 0x180U;
    const uintptr_t alias_va = hs_page_base(alias_page) + 0x180U;
    const uintptr_t old_pa = phys_page_base(target_page) + 0x180U;
    const uintptr_t new_pa = phys_page_base(VSRWX_GRWX) + 0x180U;
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t old_seed = 0x53464e43544f4c44ULL;
    const uint64_t new_seed = 0x53464e43544e4557ULL;
    const uint64_t blocked_payload = 0x53464e43424c4b01ULL;
    const uint64_t target_commit_payload = 0x53464e43434d5402ULL;
    const uint64_t target_after_rearm_payload = 0x53464e434d495303ULL;
    const uint64_t alias_blocked_payload = 0x53464e4341424b04ULL;
    const uint64_t alias_commit_payload = 0x53464e4341434d05ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    pte_t *target_pte = ai_ibc_hs_leaf_entry(target_page);
    pte_t *alias_pte = ai_ibc_hs_leaf_entry(alias_page);
    const pte_t old_leaf = (phys_page_base(target_page) >> 2) |
        PTE_V | PTE_AD | PTE_RWX;
    const pte_t new_leaf = (phys_page_base(VSRWX_GRWX) >> 2) |
        PTE_V | PTE_AD | PTE_RWX;
    const pte_t alias_leaf = old_leaf;
    volatile uint64_t marker = 0ULL;

    goto_priv(PRIV_M);
    write64(old_pa, old_seed);
    write64(new_pa, new_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRC(CSR_MIE, MIE_SSIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIDELEG, MIP_SSIP | AI_IBC_MIP_STIP);
    CSRS(CSR_MIDELEG, MIP_SSIP);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_BKP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRC(CSR_SIP, SIP_SSIP);
    CSRW(CSR_STVEC, (uintptr_t)ai_ibc_stvec_log_ssip_handler);
    hspt_init();
    *target_pte = old_leaf;
    *alias_pte = alias_leaf;
    sfence_vma();
    ai_ibc_sfence_target_pte = (uintptr_t)target_pte;
    ai_ibc_sfence_new_pte = new_leaf;
    ai_ibc_handler_mode = AI_IBC_HANDLER_MODE_SFENCE_REMAP_ON_BKP;
    ai_ibc_arm_mcontrol6_slot_priv(0, target_va,
        AI_IBC_MCONTROL6_STORE, AI_IBC_MCONTROL6_S, false);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SIP, SIP_SSIP);
    ai_ibc_store_once(target_va, blocked_payload);
    marker += 1ULL;
    const uint64_t sip_after_breakpoint = CSRR(CSR_SIP);

    goto_priv(PRIV_M);
    uint64_t tdata1_after_handler = 0ULL;
    uint64_t tdata2_after_handler = 0ULL;
    ai_ibc_read_trigger_csr_slot(0, &tdata1_after_handler,
        &tdata2_after_handler);
    const uint64_t mip_after_breakpoint = CSRR(CSR_MIP);
    const pte_t target_leaf_after_handler = *target_pte;
    const bool breakpoint_handler_remaps_and_clears_trigger =
        ai_ibc_trap_count == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == store_pc &&
        ai_ibc_trap_tval[0] == target_va &&
        ai_ibc_trap_magic[0] == AI_IBC_HS_LOG_MAGIC_BASE &&
        (ai_ibc_trap_mip[0] & SIP_SSIP) != 0ULL &&
        (sip_after_breakpoint & SIP_SSIP) != 0ULL &&
        (mip_after_breakpoint & MIP_SSIP) != 0ULL &&
        ai_ibc_sfence_handler_hits == 1ULL &&
        target_leaf_after_handler == new_leaf &&
        (tdata1_after_handler & AI_IBC_MCONTROL6_STORE) != 0ULL &&
        tdata2_after_handler == target_va &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL &&
        read64(old_pa) == old_seed &&
        read64(new_pa) == new_seed &&
        marker == 1ULL;

    ai_ibc_clear_mcontrol6_slot(0);

    goto_priv(PRIV_HS);
    CSRS(CSR_SIE, SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    const uint64_t sip_after_interrupt = CSRR(CSR_SIP);
    marker += 1ULL;

    goto_priv(PRIV_M);
    const uint64_t mip_after_interrupt = CSRR(CSR_MIP);
    const uint64_t trap_count_after_interrupt = ai_ibc_trap_count;
    const bool pending_ssip_delivers_without_stale_breakpoint =
        ai_ibc_trap_count == 2ULL &&
        ai_ibc_trap_cause[1] == CAUSE_SSI &&
        ai_ibc_trap_magic[1] == (AI_IBC_HS_LOG_MAGIC_BASE + 1ULL) &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (ai_ibc_trap_mip[1] & SIP_SSIP) != 0ULL &&
        (sip_after_interrupt & SIP_SSIP) == 0ULL &&
        (mip_after_interrupt & MIP_SSIP) == 0ULL &&
        read64(old_pa) == old_seed &&
        read64(new_pa) == new_seed &&
        marker == 2ULL;

    goto_priv(PRIV_HS);
    ai_ibc_store_once(target_va, target_commit_payload);
    marker += 1ULL;

    goto_priv(PRIV_M);
    const bool target_new_mapping_commits_without_old_va_replay =
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        read64(old_pa) == old_seed &&
        read64(new_pa) == target_commit_payload &&
        marker == 3ULL;

    ai_ibc_arm_mcontrol6_slot_priv(0, alias_va,
        AI_IBC_MCONTROL6_STORE, AI_IBC_MCONTROL6_S, false);
    goto_priv(PRIV_HS);
    ai_ibc_store_once(target_va, target_after_rearm_payload);
    marker += 1ULL;

    goto_priv(PRIV_M);
    const uint64_t trap_count_after_target_miss = ai_ibc_trap_count;
    const bool rearmed_alias_trigger_does_not_hit_target_va =
        ai_ibc_trap_count == trap_count_after_interrupt &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        read64(old_pa) == old_seed &&
        read64(new_pa) == target_after_rearm_payload &&
        marker == 4ULL;

    goto_priv(PRIV_HS);
    ai_ibc_store_once(alias_va, alias_blocked_payload);
    marker += 1ULL;

    goto_priv(PRIV_M);
    const uint64_t trap_count_after_alias_break = ai_ibc_trap_count;
    const bool alias_final_config_breaks_and_suppresses_old_pa =
        ai_ibc_trap_count == trap_count_after_target_miss + 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_trap_cause[trap_count_after_target_miss] == CAUSE_BKP &&
        ai_ibc_trap_epc[trap_count_after_target_miss] == store_pc &&
        ai_ibc_trap_tval[trap_count_after_target_miss] == alias_va &&
        ai_ibc_sfence_handler_hits == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        read64(old_pa) == old_seed &&
        read64(new_pa) == target_after_rearm_payload &&
        marker == 5ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    goto_priv(PRIV_HS);
    ai_ibc_store_once(alias_va, alias_commit_payload);
    marker += 1ULL;

    goto_priv(PRIV_M);
    const bool alias_clear_commits_without_late_interrupt_or_breakpoint =
        ai_ibc_trap_count == trap_count_after_alias_break &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause(CAUSE_SSI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_SSIP) == 0ULL &&
        read64(old_pa) == alias_commit_payload &&
        read64(new_pa) == target_after_rearm_payload &&
        marker == 6ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, SIE_SSIE | SIE_STIE);
    CSRC(CSR_MIP, MIP_SSIP | AI_IBC_MIP_STIP);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MEDELEG, saved_medeleg);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);

    TEST_ASSERT("the first HS/S VA store hits the VA address trigger once; the breakpoint handler records target VA, rewrites the target PTE, and executes sfence.vma before SSIP is delivered",
        breakpoint_handler_remaps_and_clears_trigger,
        "count=%lx cause0=%lx epc0=%lx tval0=%lx mip0=%lx sip_after=%lx pte=%lx tdata1=%lx tdata2=%lx old=%lx new=%lx hits=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_tval[0], ai_ibc_trap_mip[0], sip_after_breakpoint,
        target_leaf_after_handler, tdata1_after_handler,
        tdata2_after_handler, read64(old_pa), read64(new_pa),
        ai_ibc_sfence_handler_hits);
    TEST_ASSERT("the delegated SSIP pending during that breakpoint episode is delivered exactly once after SSIE/SIE opens and does not resurrect the old target-VA trigger match",
        pending_ssip_delivers_without_stale_breakpoint &&
        target_new_mapping_commits_without_old_va_replay,
        "count=%lx cause1=%lx magic1=%lx sip_after_irq=%lx mip_after_irq=%lx old=%lx new=%lx marker=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1], ai_ibc_trap_magic[1],
        sip_after_interrupt, mip_after_interrupt, read64(old_pa),
        read64(new_pa), marker);
    TEST_ASSERT("after rearming slot0 to alias VA B, target VA A follows the new mapping without a ghost breakpoint, while alias VA B alone reports one breakpoint and suppresses old-PA side effect",
        rearmed_alias_trigger_does_not_hit_target_va &&
        alias_final_config_breaks_and_suppresses_old_pa,
        "count=%lx target_miss_count=%lx alias_break_count=%lx cause_alias=%lx tval_alias=%lx old=%lx new=%lx hits=%lx marker=%lx",
        ai_ibc_trap_count, trap_count_after_target_miss,
        trap_count_after_alias_break,
        ai_ibc_trap_cause[trap_count_after_target_miss],
        ai_ibc_trap_tval[trap_count_after_target_miss],
        read64(old_pa), read64(new_pa), ai_ibc_sfence_handler_hits,
        marker);
    TEST_ASSERT("clearing the alias trigger lets alias VA B commit to old PA exactly once, with no late SSIP repeat or stale breakpoint replay from either VA",
        alias_clear_commits_without_late_interrupt_or_breakpoint,
        "count=%lx old=%lx new=%lx mip=%lx marker=%lx",
        ai_ibc_trap_count, read64(old_pa), read64(new_pa),
        CSRR(CSR_MIP), marker);

    TEST_END(ai_interrupt_hs_sfence_breakpoint_handler_stip_no_stale_trigger_va_corner);
}

bool ai_interrupt_m_vmodule_pmp_reconfig_flush_store_trigger_generation_contract_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x500U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x500U;
    const uintptr_t guard_paddr = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t guard_seed = 0x706d706766677501ULL;
    const uint64_t initial = 0x706d706766747202ULL;
    const uint64_t allowed_payload = 0x706d7067616c6c03ULL;
    const uint64_t blocked_payload = 0x706d7067646e7904ULL;
    const uint64_t recovered_payload = 0x706d706772637605ULL;
    const uint64_t allow_cfg =
        ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_RWX);
    const uint64_t deny_cfg =
        ai_ibc_pmp_window_cfg(AI_IBC_PMP_TOR_NO_W);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);

    goto_priv(PRIV_M);
    write64(paddr, initial);
    write64(guard_paddr, guard_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, MIE_MEIE | AI_IBC_MIE_STIE);
    CSRC(CSR_MIP, MIP_MEIP | AI_IBC_MIP_STIP);
    CSRC(CSR_MIDELEG, MIP_MEIP | AI_IBC_MIP_STIP);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_mtvec_log_handler);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(1ULL);
    ai_ibc_prepare_mprv_user_translation();
    ai_ibc_configure_pmp_window(pmp_start, pmp_end,
        AI_IBC_PMP_TOR_RWX);

    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allowed_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t old_allow_cause = excpt.cause;
    const uint64_t old_allow_tval = excpt.tval;
    const uint64_t old_allow_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t old_allow_target_after = read64(paddr);
    const uint64_t old_allow_guard_after = read64(guard_paddr);
    const bool old_allow_generation_commits =
        excpt.triggered == false &&
        old_allow_target_after == allowed_payload &&
        old_allow_guard_after == guard_seed &&
        old_allow_cfg == allow_cfg;

    ai_ibc_arm_mcontrol6_slot(0, vaddr, AI_IBC_MCONTROL6_STORE,
        false);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, pmp_start >> 2U);
    CSRW(CSR_PMPADDR1, pmp_end >> 2U);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_MAIN_INTERRUPT,
        AI_IBC_VMODULE_MAIN_MEI);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t half_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t half_addr0 = CSRR(CSR_PMPADDR0);
    const uint64_t half_addr1 = CSRR(CSR_PMPADDR1);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, deny_cfg);
    const uint64_t deny_cfg_after_write = CSRR(CSR_PMPCFG0);

    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, blocked_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t deny_breakpoint_mip_after = CSRR(CSR_MIP);
    const uint64_t deny_breakpoint_target_after = read64(paddr);
    const uint64_t deny_breakpoint_guard_after = read64(guard_paddr);
    const bool deny_generation_breakpoint_suppresses_store =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_BKP &&
        excpt.tval == vaddr &&
        deny_cfg_after_write == deny_cfg &&
        half_cfg == 0ULL &&
        half_addr0 == (pmp_start >> 2U) &&
        half_addr1 == (pmp_end >> 2U) &&
        deny_breakpoint_target_after == allowed_payload &&
        deny_breakpoint_guard_after == guard_seed &&
        ai_ibc_log_has_cause_tval_pmp_snapshot(CAUSE_BKP, vaddr,
            deny_cfg, pmp_start >> 2U, pmp_end >> 2U) &&
        (deny_breakpoint_mip_after & MIP_MEIP) != 0ULL;

    ai_ibc_clear_mcontrol6_slot(0);
    CSRS(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const uint64_t mei_mip_after = CSRR(CSR_MIP);
    const uint64_t mei_target_after = read64(paddr);
    const uint64_t mei_guard_after = read64(guard_paddr);
    const bool vmodule_mei_logs_same_deny_generation =
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_has_cause_pmp_snapshot(CAUSE_MEI, deny_cfg,
            pmp_start >> 2U, pmp_end >> 2U) &&
        (mei_mip_after & MIP_MEIP) == 0ULL &&
        mei_target_after == allowed_payload &&
        mei_guard_after == guard_seed;

    CSRW(CSR_MTVEC, saved_mtvec);
    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, blocked_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t deny_fault_cause = excpt.cause;
    const uint64_t deny_fault_tval = excpt.tval;
    const uint64_t deny_fault_cfg = CSRR(CSR_PMPCFG0);
    const uint64_t deny_fault_target_after = read64(paddr);
    const uint64_t deny_fault_guard_after = read64(guard_paddr);
    const bool clear_trigger_keeps_deny_fault_generation =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        deny_fault_cause == CAUSE_SAF &&
        deny_fault_tval == vaddr &&
        deny_fault_cfg == deny_cfg &&
        deny_fault_target_after == allowed_payload &&
        deny_fault_guard_after == guard_seed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL;

    ai_ibc_configure_pmp_window(pmp_start, pmp_end,
        AI_IBC_PMP_TOR_RWX);
    ai_ibc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, recovered_payload);
    ai_ibc_clear_mprv_translation();
    const uint64_t restore_cfg_after = CSRR(CSR_PMPCFG0);
    const uint64_t restore_target_after = read64(paddr);
    const uint64_t restore_guard_after = read64(guard_paddr);
    const uint64_t restore_mip_after = CSRR(CSR_MIP);
    const bool restore_generation_commits_after_flush =
        excpt.triggered == false &&
        restore_cfg_after == allow_cfg &&
        restore_target_after == recovered_payload &&
        restore_guard_after == guard_seed &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (restore_mip_after & MIP_MEIP) == 0ULL;

    goto_priv(PRIV_M);
    ai_ibc_clear_mprv_translation();
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, MIP_MEIP | AI_IBC_MIP_STIP);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, saved_pmpaddr0);
    CSRW(CSR_PMPADDR1, saved_pmpaddr1);
    CSRW(CSR_PMPADDR2, saved_pmpaddr2);
    CSRW(CSR_PMPCFG0, saved_pmpcfg0);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("old PMP allow generation is reachable through MPRV=U before the reconfiguration window starts",
        old_allow_generation_commits,
        "cause=%lx tval=%lx cfg=%lx target=%lx guard=%lx",
        old_allow_cause, old_allow_tval, old_allow_cfg,
        old_allow_target_after, old_allow_guard_after);
    TEST_ASSERT("after pmpaddr staging and final deny cfg, the store-address breakpoint logs the denied PMP generation and suppresses the store while VModule MEIP stays pending",
        deny_generation_breakpoint_suppresses_store,
        "count=%lx cfg_half=%lx addr0=%lx addr1=%lx cfg_final=%lx cause0=%lx tval0=%lx pmp0=%lx target=%lx mip=%lx",
        ai_ibc_trap_count, half_cfg, half_addr0, half_addr1,
        deny_cfg_after_write, ai_ibc_trap_cause[0],
        ai_ibc_trap_tval[0], ai_ibc_trap_pmpcfg0[0],
        deny_breakpoint_target_after, deny_breakpoint_mip_after);
    TEST_ASSERT("when MEIE/MIE opens, the VModule MEI handler observes the same complete deny PMP snapshot and clears the external source exactly once",
        vmodule_mei_logs_same_deny_generation,
        "count=%lx cause0=%lx cause1=%lx pmp_bkp=%lx pmp_mei=%lx addr0_mei=%lx addr1_mei=%lx mip=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0],
        ai_ibc_trap_cause[1], ai_ibc_trap_pmpcfg0[0],
        ai_ibc_trap_pmpcfg0[1], ai_ibc_trap_pmpaddr0[1],
        ai_ibc_trap_pmpaddr1[1], mei_mip_after);
    TEST_ASSERT("after clearing only the trigger, the same translated store reports store access fault under the denied PMP generation without a stale side effect",
        clear_trigger_keeps_deny_fault_generation,
        "cause=%lx tval=%lx cfg=%lx target=%lx guard=%lx",
        deny_fault_cause, deny_fault_tval, deny_fault_cfg,
        deny_fault_target_after, deny_fault_guard_after);
    TEST_ASSERT("restoring PMP RWX and flushing the translated path lets the retry store commit once with no late VModule interrupt or ghost breakpoint",
        restore_generation_commits_after_flush,
        "cfg=%lx target=%lx guard=%lx count=%lx mip=%lx",
        restore_cfg_after, restore_target_after, restore_guard_after,
        ai_ibc_trap_count, restore_mip_after);

    TEST_END(ai_interrupt_m_vmodule_pmp_reconfig_flush_store_trigger_generation_contract_corner);
}

bool ai_interrupt_m_vmodule_rnmi_mtvec_switch_vector_isolation_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_mtvec_switch_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_ebreak_probe();
    const uint64_t mtvec_after_breakpoint = CSRR(CSR_MTVEC);
    const bool breakpoint_switches_mtvec_and_rnmi_uses_mn_state =
        ai_ibc_rnmi_switch_bp_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_ebreak_probe) == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        (ai_ibc_rnmi_switch_rnmi_entry_id == 0x524e4d4953564301ULL ||
            ai_ibc_rnmi_switch_rnmi_entry_id == 0x524e4d4953564302ULL) &&
        ai_ibc_rnmi_switch_mtvec_after_bkp ==
            (uintptr_t)ai_ibc_rnmi_mtvec_switch_alt_mei_handler &&
        mtvec_after_breakpoint ==
            (uintptr_t)ai_ibc_rnmi_mtvec_switch_alt_mei_handler &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_rnmi_switch_unexpected_hits == 0ULL;

    ai_ibc_prepare_vmodule_meip_pending(true, 0ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT &&
        ai_ibc_rnmi_switch_alt_mei_hits == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    CSRC(CSR_MSTATUS, MSTATUS_MIE);

    const bool ordinary_mei_uses_switched_mtvec_after_rnmi_return =
        ai_ibc_rnmi_switch_alt_mei_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_trap_magic[1] == (0x494243414c544d00ULL + 1ULL) &&
        ai_ibc_rnmi_switch_alt_mtvec_seen ==
            (uintptr_t)ai_ibc_rnmi_mtvec_switch_alt_mei_handler &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        ai_ibc_rnmi_switch_unexpected_hits == 0ULL;

    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool sources_clear_without_ghost_vector_entry =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_switch_bp_hits == 1ULL &&
        ai_ibc_rnmi_switch_alt_mei_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_MEI) == 1ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL &&
        ai_ibc_rnmi_switch_unexpected_hits == 0ULL;

    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_clear_main_interrupt();
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("while the breakpoint handler switches mtvec, VModule NMI_31 still records through RNMI CSR state and not the ordinary trap log",
        breakpoint_switches_mtvec_and_rnmi_uses_mn_state,
        "count=%lx bkp_hits=%lx rnmi_hits=%lx rnmi_cause=%lx entry=%lx mtvec_after=%lx mret=%lx mnret=%lx",
        ai_ibc_trap_count, ai_ibc_rnmi_switch_bp_hits,
        ai_ibc_rnmi_hits, ai_ibc_rnmi_mncause[0],
        ai_ibc_rnmi_switch_rnmi_entry_id, mtvec_after_breakpoint,
        ai_ibc_rnmi_mret_markers, ai_ibc_rnmi_mnret_markers);
    TEST_ASSERT("after mnret returns to the breakpoint flow, the later VModule MEI uses the switched ordinary mtvec exactly once and clears vm_reg_0",
        ordinary_mei_uses_switched_mtvec_after_rnmi_return,
        "count=%lx cause1=%lx magic1=%lx alt_hits=%lx alt_mtvec=%lx mip=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[1],
        ai_ibc_trap_magic[1], ai_ibc_rnmi_switch_alt_mei_hits,
        ai_ibc_rnmi_switch_alt_mtvec_seen, CSRR(CSR_MIP));
    TEST_ASSERT("clearing the VModule RNMI and MEI sources leaves exactly one breakpoint, one RNMI, and one ordinary interrupt entry with no ghost vector reuse",
        sources_clear_without_ghost_vector_entry);

    TEST_END(ai_interrupt_m_vmodule_rnmi_mtvec_switch_vector_isolation_corner);
}

bool ai_interrupt_m_vmodule_rnmi_fetch_block_no_ghost_breakpoint_corner()
{
    TEST_START();

    const uintptr_t target_addr = (uintptr_t)&ai_ibc_mem[0];
    const uintptr_t guard_addr = (uintptr_t)&ai_ibc_mem[1];
    const uintptr_t store_pc = (uintptr_t)ai_ibc_store_once;
    const uint64_t target_seed = 0x524e4d4946434801ULL;
    const uint64_t guard_seed = 0x524e4d4946434702ULL;
    const uint64_t rnmi_window_payload = 0x524e4d4946434203ULL;
    const uint64_t explicit_block_payload = 0x524e4d4946434204ULL;
    const uint64_t final_payload = 0x524e4d4946434605ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    write64(target_addr, target_seed);
    write64(guard_addr, guard_seed);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(16ULL);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_rnmi_bkp_csr_owner_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_arm_mcontrol6_slot(0, store_pc, AI_IBC_MCONTROL6_EXECUTE,
        false);
    ai_ibc_vmodule_set_delay(1ULL);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI,
        AI_IBC_VMODULE_NMI_31);
    ai_ibc_store_once(target_addr, rnmi_window_payload);
    for (uint64_t i = 0; i < AI_IBC_VMODULE_POLL_LIMIT; ++i) {
        if (ai_ibc_rnmi_hits >= 1ULL &&
            ai_ibc_log_count_cause(CAUSE_BKP) >= 1ULL) {
            break;
        }
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool rnmi_vector_does_not_consume_old_breakpoint_metadata =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == AI_IBC_NMI31_CAUSE &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 1ULL &&
        ai_ibc_log_count_cause(AI_IBC_NMI31_CAUSE) == 0ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed;

    ai_ibc_store_once(target_addr, explicit_block_payload);
    for (uint64_t i = 0; i < AI_IBC_SPIN_LIMIT &&
        ai_ibc_log_count_cause(CAUSE_BKP) < 2ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    const bool explicit_revisit_still_hits_current_breakpoint =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP, store_pc) == 2ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        read64(target_addr) == target_seed &&
        read64(guard_addr) == guard_seed;

    ai_ibc_clear_mcontrol6_slot(0);
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_store_once(target_addr, final_payload);
    const bool clearing_trigger_and_source_allows_plain_commit =
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 2ULL &&
        ai_ibc_rnmi_mret_markers == 2ULL &&
        read64(target_addr) == final_payload &&
        read64(guard_addr) == guard_seed;

    ai_ibc_clear_all_mcontrol6_slots();
    ai_ibc_vmodule_write_reg(AI_IBC_VMODULE_REG_FORCE_NMI, 0ULL);
    ai_ibc_vmodule_set_delay(0ULL);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("VModule NMI_31 entering RNMI while an execute trigger is armed does not turn RNMI vector fetch into a ghost ordinary breakpoint event",
        rnmi_vector_does_not_consume_old_breakpoint_metadata,
        "rnmi=%lx mnret=%lx count=%lx bkp=%lx target=%lx guard=%lx",
        ai_ibc_rnmi_hits, ai_ibc_rnmi_mnret_markers,
        ai_ibc_trap_count, ai_ibc_log_count_cause(CAUSE_BKP),
        read64(target_addr), read64(guard_addr));
    TEST_ASSERT("explicitly executing the old breakpoint block after RNMI return still reports the current store-PC breakpoint exactly once more",
        explicit_revisit_still_hits_current_breakpoint,
        "count=%lx mret=%lx target=%lx guard=%lx",
        ai_ibc_trap_count, ai_ibc_rnmi_mret_markers,
        read64(target_addr), read64(guard_addr));
    TEST_ASSERT("after clearing both VModule NMI source and the trigger slot, the same store helper commits once with no late RNMI or breakpoint replay",
        clearing_trigger_and_source_allows_plain_commit);

    TEST_END(ai_interrupt_m_vmodule_rnmi_fetch_block_no_ghost_breakpoint_corner);
}

bool ai_interrupt_m_unexpected_trap_rnmi_breakpoint_handler_fault_owner_corner()
{
    TEST_START();

    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);

    goto_priv(PRIV_M);
    ai_ibc_reset_log();
    ai_ibc_clear_all_mcontrol6_slots();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIE, 0ULL);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_ibc_unexpected_rnmi_bkp_fault_handler);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);

    ai_ibc_ebreak_probe();

    const bool breakpoint_record_keeps_ordinary_owner =
        ai_ibc_unexpected_rnmi_bkp_hits == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL &&
        ai_ibc_log_count_cause_epc(CAUSE_BKP,
            (uintptr_t)ai_ibc_ebreak_probe) == 1ULL &&
        ai_ibc_trap_cause[0] == CAUSE_BKP &&
        ai_ibc_trap_epc[0] == (uintptr_t)ai_ibc_ebreak_probe &&
        ai_ibc_trap_magic[0] == 0x494243554e455800ULL;
    const bool unexpected_fault_uses_rnmi_owner_if_platform_routes_it =
        ai_ibc_unexpected_rnmi_fault_hits == 1ULL &&
        ai_ibc_rnmi_hits == 1ULL &&
        ai_ibc_rnmi_mncause[0] == CAUSE_ILI &&
        ai_ibc_rnmi_mnepc[0] != 0ULL &&
        ai_ibc_rnmi_mnepc[0] != ai_ibc_trap_epc[0] &&
        (ai_ibc_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_ibc_rnmi_mnret_markers == 1ULL &&
        ai_ibc_rnmi_mret_markers == 1ULL &&
        ai_ibc_unexpected_rnmi_after_fault_marker == 1ULL &&
        ai_ibc_unexpected_rnmi_unexpected_hits == 0ULL;
    const bool fallback_nested_ordinary_fault_is_visible_not_silent =
        ai_ibc_unexpected_rnmi_fault_hits == 0ULL &&
        ai_ibc_rnmi_hits == 0ULL &&
        ai_ibc_log_count_cause(CAUSE_ILI) == 1ULL &&
        ai_ibc_trap_cause[1] == CAUSE_ILI &&
        ai_ibc_trap_epc[1] != ai_ibc_trap_epc[0] &&
        ai_ibc_unexpected_rnmi_after_fault_marker == 1ULL &&
        ai_ibc_unexpected_rnmi_unexpected_hits == 1ULL;
    const bool handler_fault_has_single_explicit_owner =
        unexpected_fault_uses_rnmi_owner_if_platform_routes_it ||
        fallback_nested_ordinary_fault_is_visible_not_silent;

    CSRW(CSR_MTVEC, saved_mtvec);
    rnmi_handler_enabled = false;
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    ai_ibc_restore_m_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mtvec, saved_mepc, saved_menvcfg);

    TEST_ASSERT("the original breakpoint handler entry keeps its ordinary mcause/mepc owner before the controlled handler fault is injected",
        breakpoint_record_keeps_ordinary_owner,
        "count=%lx cause0=%lx epc0=%lx magic0=%lx bkp_hits=%lx",
        ai_ibc_trap_count, ai_ibc_trap_cause[0], ai_ibc_trap_epc[0],
        ai_ibc_trap_magic[0], ai_ibc_unexpected_rnmi_bkp_hits);
    TEST_ASSERT("the controlled illegal fault inside the breakpoint handler has one explicit owner: RNMI CSR when double-trap routes there, otherwise a visible nested ordinary illegal record",
        handler_fault_has_single_explicit_owner,
        "rnmi_hits=%lx rnmi_cause=%lx rnmi_epc=%lx fault_hits=%lx ordinary_ili=%lx marker=%lx unexpected=%lx",
        ai_ibc_rnmi_hits, ai_ibc_rnmi_mncause[0],
        ai_ibc_rnmi_mnepc[0], ai_ibc_unexpected_rnmi_fault_hits,
        ai_ibc_log_count_cause(CAUSE_ILI),
        ai_ibc_unexpected_rnmi_after_fault_marker,
        ai_ibc_unexpected_rnmi_unexpected_hits);
    TEST_ASSERT("the handler returns after the controlled fault without overwriting the original breakpoint CSR record or looping the fault site",
        ai_ibc_unexpected_rnmi_bkp_hits == 1ULL &&
        ai_ibc_unexpected_rnmi_after_fault_marker == 1ULL &&
        ai_ibc_log_count_cause(CAUSE_BKP) == 1ULL);

    TEST_END(ai_interrupt_m_unexpected_trap_rnmi_breakpoint_handler_fault_owner_corner);
}
