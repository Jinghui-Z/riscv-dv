#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

#define AI_IMC_MIE_STIE (1ULL << 5)
#define AI_IMC_MIP_STIP (1ULL << 5)
#define AI_IMC_MIE_MSIE (1ULL << 3)
#define AI_IMC_MIP_MSIP (1ULL << 3)
#define AI_IMC_SIE_SSIE (1ULL << 1)
#define AI_IMC_SIP_SSIP (1ULL << 1)
#define AI_IMC_SPIN_LIMIT 1024ULL
#define AI_IMC_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_IMC_MSTATUS_MPP_M (3ULL << 11)
#define AI_IMC_PMP_TOR_RWX 0x0fULL
#define AI_IMC_PMP_TOR_NO_R 0x0cULL
#define AI_IMC_PMP_TOR_NO_W 0x0dULL
#define AI_IMC_PMP_NAPOT_RWX 0x1fULL
#define AI_IMC_PMP_NAPOT_NO_W 0x1dULL
#define AI_IMC_LOG_SLOTS 4U
#define AI_IMC_MSTATUS_VS_MASK (3ULL << 9)
#define AI_IMC_MSTATUS_VS_CLEAN (2ULL << 9)
#define AI_IMC_MSTATUS_VS_DIRTY (3ULL << 9)
#define AI_IMC_VEC_VL4 4U
#define AI_IMC_VEC_VL6 6U
#define AI_IMC_VEC_VL8 8U
#define AI_IMC_VEC_SEG2_BYTES 8U
#define AI_IMC_FAST_REPLAY_VEC_BYTES 16U
#define AI_IMC_VEC_WHOLE_MAX_BYTES 4096U
#define AI_IMC_VFOF_FULL_VL 16U
#define AI_IMC_VFOF_PREFIX_BYTES 8U
#define AI_IMC_MASK_VL9 9U
#define AI_IMC_MASK_BYTES 2U
#define AI_IMC_MASK_E8_BYTES 12U
#define AI_IMC_MASK_E16_ELEMS 10U
#define AI_IMC_SATP_MODE_SV39 (8ULL << 60)
#define AI_IMC_SATP_ASID_SHIFT 44U
#define AI_IMC_SATP_ASID_MASK (0xffffULL << AI_IMC_SATP_ASID_SHIFT)
#define AI_IMC_SATP_ASID_A 0x0311ULL
#define AI_IMC_SATP_ASID_B 0x0523ULL
#define AI_IMC_SV39_L2_INDEX(addr) (((addr) >> 30U) & 0x1ffULL)
#define AI_IMC_SV39_L1_INDEX(addr) (((addr) >> 21U) & 0x1ffULL)
#define AI_IMC_SV39_L0_INDEX(addr) (((addr) >> 12U) & 0x1ffULL)
#define AI_IMC_PBMT_IO_MEM_BASE  0x30000000000ULL
#define AI_IMC_PBMT_IO_MEM_LIMIT 0x40000000000ULL
#define AI_IMC_PMA_R      (1ULL << 0)
#define AI_IMC_PMA_W      (1ULL << 1)
#define AI_IMC_PMA_X      (1ULL << 2)
#define AI_IMC_PMA_A_TOR  (1ULL << 3)
#define AI_IMC_PMA_ATOMIC (1ULL << 5)
#define AI_IMC_PMA_C      (1ULL << 6)
#ifndef CLINT_BASE
#define CLINT_BASE 0x02000000UL
#endif
#define AI_IMC_CLINT_MSIP0 (CLINT_BASE + 0x0000U)
#ifndef CSR_SPFCTL
#define CSR_SPFCTL 0x5c1
#endif
#define AI_IMC_SPFCTL_L2_PF_ENABLE          (1ULL << 1)
#define AI_IMC_SPFCTL_L1D_PF_ENABLE         (1ULL << 2)
#define AI_IMC_SPFCTL_TRAIN_ON_HIT          (1ULL << 3)
#define AI_IMC_SPFCTL_ENABLE_AGT            (1ULL << 4)
#define AI_IMC_SPFCTL_ENABLE_PHT            (1ULL << 5)
#define AI_IMC_SPFCTL_ACTIVE_THRESHOLD_MASK (0xFULL << 6)
#define AI_IMC_SPFCTL_ACTIVE_STRIDE_MASK    (0x3FULL << 10)
#define AI_IMC_SPFCTL_ACTIVE_THRESHOLD_12   (0xcULL << 6)
#define AI_IMC_SPFCTL_ACTIVE_STRIDE_1       (0x1ULL << 10)

extern void ai_imc_mtvec_log_handler(void);
extern void ai_imc_mtvec_same_target_load_handler(void);
extern void ai_imc_mtvec_same_pc_helper_load_handler(void);
extern void ai_imc_mtvec_partial_store_handler(void);
extern void ai_imc_mtvec_mprv_save_handler(void);
extern void ai_imc_mtvec_amo_log_handler(void);
extern void ai_imc_mtvec_pma_atomic_retry_generation_handler(void);
extern void ai_imc_mtvec_clint_msip_clear_handler(void);
extern void ai_imc_mtvec_spill_stack_handler(void);
extern void ai_imc_mtvec_retry_store_handler(void);
extern void ai_imc_mtvec_block_copy_repair_handler(void);
extern void ai_imc_mtvec_message_ack_handler(void);
extern void ai_imc_mtvec_time_log_handler(void);
extern void ai_imc_mtvec_cbozero_handler(void);
extern void ai_imc_mtvec_vector_context_handler(void);
extern void ai_imc_mtvec_mask_context_handler(void);
extern void ai_imc_mtvec_segment_spill_handler(void);
extern void ai_imc_stvec_same_target_load_handler(void);
extern void ai_imc_stvec_stack_fault_repair_handler(void);
extern void ai_imc_stvec_nested_fault_preserve_retry_handler(void);
extern void ai_imc_mprv_store_retry_probe(uintptr_t addr, uint64_t value);
extern void ai_imc_cbozero_once(uintptr_t addr);
extern uint64_t ai_imc_same_pc_load_helper(uintptr_t addr,
    volatile uintptr_t *pc_log);

volatile uint64_t ai_imc_trap_count;
volatile uint64_t ai_imc_trap_cause[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_trap_epc[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_trap_tval[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_trap_mip[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_log_page[AI_IMC_LOG_SLOTS]
    __attribute__((aligned(64)));
volatile uintptr_t ai_imc_handler_probe_addr;
volatile uintptr_t ai_imc_handler_store_addr;
volatile uint64_t ai_imc_handler_store_value;
volatile uint64_t ai_imc_handler_probe_value[AI_IMC_LOG_SLOTS];
volatile uintptr_t ai_imc_same_pc_helper_pc[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_mstatus[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_amo_addend;
volatile uint64_t ai_imc_handler_amo_old[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_amo_final[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_spill_stack[AI_IMC_LOG_SLOTS * 4U]
    __attribute__((aligned(64)));
volatile uintptr_t ai_imc_handler_saved_sp[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_spill_guard0;
volatile uint64_t ai_imc_handler_spill_guard1;
volatile uintptr_t ai_imc_handler_cbo_addr;
volatile uintptr_t ai_imc_retry_store_pc;
volatile uintptr_t ai_imc_retry_after_pc;
volatile uint64_t ai_imc_retry_fault_repairs;
volatile uint64_t ai_imc_retry_pmpaddr0;
volatile uint64_t ai_imc_retry_pmpaddr1;
volatile uint64_t ai_imc_retry_pmpaddr2;
volatile uintptr_t ai_imc_block_copy_src_addr;
volatile uintptr_t ai_imc_block_copy_dst_addr;
volatile uint64_t ai_imc_block_copy_words;
volatile uintptr_t ai_imc_block_copy_pte_addr;
volatile uint64_t ai_imc_block_copy_pte_value;
volatile uintptr_t ai_imc_block_copy_restore_mtvec;
volatile uint64_t ai_imc_block_copy_repairs;
volatile uintptr_t ai_imc_message_seq_addr;
volatile uintptr_t ai_imc_message_payload_addr;
volatile uintptr_t ai_imc_message_checksum_addr;
volatile uintptr_t ai_imc_message_ack_addr;
volatile uint64_t ai_imc_message_seq_log[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_message_payload_log[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_message_checksum_log[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_message_ack_log[AI_IMC_LOG_SLOTS];
volatile uintptr_t ai_imc_time_log_store_addr;
volatile uint64_t ai_imc_time_log_value[AI_IMC_LOG_SLOTS];
volatile uintptr_t ai_imc_handler_vec_src_addr;
volatile uintptr_t ai_imc_handler_vec_dst_addr;
volatile uintptr_t ai_imc_handler_vec_save_addr;
volatile uint64_t ai_imc_handler_vec_saved_vl[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_vec_saved_vtype[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_vec_saved_vstart[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_vec_handler_vl[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_handler_vec_handler_vtype[AI_IMC_LOG_SLOTS];
volatile uintptr_t ai_imc_handler_mask_save_addr;
volatile uintptr_t ai_imc_handler_mask_src_addr;
volatile uintptr_t ai_imc_handler_mask_dump_addr;
volatile uintptr_t ai_imc_handler_mask_payload_addr;
volatile uintptr_t ai_imc_handler_mask_target_addr;
volatile uintptr_t ai_imc_stack_fault_spill_base;
volatile uintptr_t ai_imc_stack_fault_saved_sp;
volatile uint64_t ai_imc_stack_fault_saved_sstatus;
volatile uintptr_t ai_imc_stack_fault_main_pte_addr;
volatile uint64_t ai_imc_stack_fault_main_pte_value;
volatile uintptr_t ai_imc_stack_fault_spill_pte_addr;
volatile uint64_t ai_imc_stack_fault_spill_pte_value;
volatile uint64_t ai_imc_stack_fault_in_interrupt;
volatile uint64_t ai_imc_stack_fault_main_repairs;
volatile uint64_t ai_imc_stack_fault_spill_repairs;
volatile uintptr_t ai_imc_nested_fault_main_pte_addr;
volatile uint64_t ai_imc_nested_fault_main_pte_value;
volatile uintptr_t ai_imc_nested_fault_handler_pte_addr;
volatile uint64_t ai_imc_nested_fault_handler_pte_value;
volatile uintptr_t ai_imc_nested_fault_handler_store_addr;
volatile uint64_t ai_imc_nested_fault_handler_store_value;
volatile uint64_t ai_imc_nested_fault_in_handler;
volatile uint64_t ai_imc_nested_fault_main_repairs;
volatile uint64_t ai_imc_nested_fault_handler_repairs;
volatile uint64_t ai_imc_nested_fault_main_saved_sstatus;
volatile uint64_t ai_imc_nested_fault_ssip_saved_sstatus;
volatile uintptr_t ai_imc_pma_retry_target_addr;
volatile uint64_t ai_imc_pma_retry_pmaaddr0;
volatile uint64_t ai_imc_pma_retry_pmaaddr1;
volatile uint64_t ai_imc_pma_retry_allow_cfg;
volatile uint64_t ai_imc_pma_retry_fault_repairs;
volatile uint64_t ai_imc_pma_retry_interrupts;
volatile uint64_t ai_imc_pma_retry_value_on_fault;
volatile uint64_t ai_imc_pma_retry_pmacfg_seen_in_interrupt;
volatile uint64_t ai_imc_pma_retry_target_after_complete;
volatile uintptr_t ai_imc_clint_msip_addr;
volatile uintptr_t ai_imc_clint_handler_log_addr;
volatile uint64_t ai_imc_clint_msip_before_clear[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_clint_msip_after_clear[AI_IMC_LOG_SLOTS];
volatile uint64_t ai_imc_clint_log_value[AI_IMC_LOG_SLOTS];
static uint8_t ai_imc_vec_whole_src[AI_IMC_VEC_WHOLE_MAX_BYTES]
    __attribute__((aligned(64)));
static uint8_t ai_imc_vec_whole_dst[AI_IMC_VEC_WHOLE_MAX_BYTES + 16U]
    __attribute__((aligned(64)));
static pte_t ai_imc_satp_root_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l1_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l2_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l3_old[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_root_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l1_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l2_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_satp_l3_new[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_super_sv39_root[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_super_sv39_l1[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imc_super_sv39_l0[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_block_copy_repair_handler\n"
    "ai_imc_mtvec_block_copy_repair_handler:\n"
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
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_bc_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_bc_log_done:\n"
    "  li t4, 13\n"
    "  beq t0, t4, .Lai_imc_bc_do_copy\n"
    "  srli t4, t0, 63\n"
    "  beqz t4, .Lai_imc_bc_restore_mtvec\n"
    "  andi t4, t0, 63\n"
    "  li t6, 5\n"
    "  bne t4, t6, .Lai_imc_bc_restore_mtvec\n"
    "  li t6, 1\n"
    "  sll t6, t6, t4\n"
    "  csrc mip, t6\n"
    "  j .Lai_imc_bc_restore_mtvec\n"
    ".Lai_imc_bc_do_copy:\n"
    "  la t4, ai_imc_block_copy_repairs\n"
    "  ld t5, 0(t4)\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    "  la t4, ai_imc_block_copy_src_addr\n"
    "  ld t0, 0(t4)\n"
    "  la t4, ai_imc_block_copy_dst_addr\n"
    "  ld t1, 0(t4)\n"
    "  la t4, ai_imc_block_copy_words\n"
    "  ld t2, 0(t4)\n"
    ".Lai_imc_bc_copy_loop:\n"
    "  beqz t2, .Lai_imc_bc_copy_done\n"
    "  ld t3, 0(t0)\n"
    "  sd t3, 0(t1)\n"
    "  addi t0, t0, 8\n"
    "  addi t1, t1, 8\n"
    "  addi t2, t2, -1\n"
    "  j .Lai_imc_bc_copy_loop\n"
    ".Lai_imc_bc_copy_done:\n"
    "  fence rw, rw\n"
    "  la t4, ai_imc_block_copy_pte_addr\n"
    "  ld t0, 0(t4)\n"
    "  beqz t0, .Lai_imc_bc_restore_mtvec\n"
    "  la t4, ai_imc_block_copy_pte_value\n"
    "  ld t1, 0(t4)\n"
    "  sd t1, 0(t0)\n"
    "  sfence.vma\n"
    "  fence rw, rw\n"
    ".Lai_imc_bc_restore_mtvec:\n"
    "  la t4, ai_imc_block_copy_restore_mtvec\n"
    "  ld t5, 0(t4)\n"
    "  beqz t5, .Lai_imc_bc_restore_regs\n"
    "  csrw mtvec, t5\n"
    ".Lai_imc_bc_restore_regs:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n"
    "\n"
    ".align 2\n"
    ".global ai_imc_mtvec_log_handler\n"
    "ai_imc_mtvec_log_handler:\n"
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
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 1f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "1:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_same_pc_load_helper\n"
    "ai_imc_same_pc_load_helper:\n"
    "  auipc t0, 0\n"
    "  sd t0, 0(a1)\n"
    "  ld a0, 0(a0)\n"
    "  ret\n"
    "\n"
    ".align 2\n"
    ".global ai_imc_mtvec_same_pc_helper_load_handler\n"
    "ai_imc_mtvec_same_pc_helper_load_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd ra, 0(sp)\n"
    "  sd t0, 8(sp)\n"
    "  sd t1, 16(sp)\n"
    "  sd t2, 24(sp)\n"
    "  sd t3, 32(sp)\n"
    "  sd t4, 40(sp)\n"
    "  sd t5, 48(sp)\n"
    "  sd t6, 56(sp)\n"
    "  sd a0, 64(sp)\n"
    "  sd a1, 72(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  csrr t3, mip\n"
    "  sd t0, 80(sp)\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_sph_clear\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_handler_probe_addr\n"
    "  ld a0, 0(t4)\n"
    "  beqz a0, .Lai_imc_sph_zero_probe\n"
    "  la a1, ai_imc_same_pc_helper_pc\n"
    "  add a1, a1, t6\n"
    "  call ai_imc_same_pc_load_helper\n"
    "  j .Lai_imc_sph_store_probe\n"
    ".Lai_imc_sph_zero_probe:\n"
    "  li a0, 0\n"
    ".Lai_imc_sph_store_probe:\n"
    "  la t4, ai_imc_handler_probe_value\n"
    "  add t4, t4, t6\n"
    "  sd a0, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_sph_clear:\n"
    "  ld t0, 80(sp)\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld ra, 0(sp)\n"
    "  ld t0, 8(sp)\n"
    "  ld t1, 16(sp)\n"
    "  ld t2, 24(sp)\n"
    "  ld t3, 32(sp)\n"
    "  ld t4, 40(sp)\n"
    "  ld t5, 48(sp)\n"
    "  ld t6, 56(sp)\n"
    "  ld a0, 64(sp)\n"
    "  ld a1, 72(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n"
    "\n"
    ".align 2\n"
    ".global ai_imc_stvec_stack_fault_repair_handler\n"
    "ai_imc_stvec_stack_fault_repair_handler:\n"
    "  addi sp, sp, -96\n"
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
    "  csrr t3, sip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_sfr_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t4, 0x696d637373667200\n"
    "  add t4, t4, t5\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  slli t3, t5, 3\n"
    "  add t6, t6, t3\n"
    "  sd t4, 0(t6)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_sfr_log_done:\n"
    "  srli t3, t0, 63\n"
    "  bnez t3, .Lai_imc_sfr_interrupt\n"
    "  li t3, 15\n"
    "  bne t0, t3, .Lai_imc_sfr_restore_regs\n"
    "  la t3, ai_imc_stack_fault_in_interrupt\n"
    "  ld t4, 0(t3)\n"
    "  bnez t4, .Lai_imc_sfr_spill_fault\n"
    "  la t3, ai_imc_stack_fault_main_repairs\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t3, ai_imc_stack_fault_main_pte_addr\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, .Lai_imc_sfr_restore_regs\n"
    "  la t3, ai_imc_stack_fault_main_pte_value\n"
    "  ld t6, 0(t3)\n"
    "  sd t6, 0(t4)\n"
    "  sfence.vma\n"
    "  j .Lai_imc_sfr_restore_regs\n"
    ".Lai_imc_sfr_spill_fault:\n"
    "  la t3, ai_imc_stack_fault_spill_repairs\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t3, ai_imc_stack_fault_spill_pte_addr\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, .Lai_imc_sfr_restore_regs\n"
    "  la t3, ai_imc_stack_fault_spill_pte_value\n"
    "  ld t6, 0(t3)\n"
    "  sd t6, 0(t4)\n"
    "  sfence.vma\n"
    "  j .Lai_imc_sfr_restore_regs\n"
    ".Lai_imc_sfr_interrupt:\n"
    "  andi t3, t0, 63\n"
    "  li t4, 1\n"
    "  bne t3, t4, .Lai_imc_sfr_clear_interrupt\n"
    "  la t4, ai_imc_stack_fault_saved_sp\n"
    "  sd sp, 0(t4)\n"
    "  csrr t6, sstatus\n"
    "  la t4, ai_imc_stack_fault_saved_sstatus\n"
    "  sd t6, 0(t4)\n"
    "  la t4, ai_imc_stack_fault_in_interrupt\n"
    "  li t6, 1\n"
    "  sd t6, 0(t4)\n"
    "  la t4, ai_imc_stack_fault_spill_base\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_sfr_restore_stack_before_clear\n"
    "  mv sp, t4\n"
    "  li t6, 0x696d637373663031\n"
    "  sd t6, 0(sp)\n"
    "  li t6, 0x696d637373663132\n"
    "  sd t6, 8(sp)\n"
    "  li t6, 0x696d637373663233\n"
    "  sd t6, 16(sp)\n"
    "  li t6, 0x696d637373663334\n"
    "  sd t6, 24(sp)\n"
    "  fence rw, rw\n"
    ".Lai_imc_sfr_restore_stack_before_clear:\n"
    "  la t4, ai_imc_stack_fault_saved_sp\n"
    "  ld sp, 0(t4)\n"
    "  la t4, ai_imc_stack_fault_in_interrupt\n"
    "  sd zero, 0(t4)\n"
    ".Lai_imc_sfr_clear_interrupt:\n"
    "  andi t3, t0, 63\n"
    "  li t4, 1\n"
    "  sll t4, t4, t3\n"
    "  csrc sip, t4\n"
    "  la t4, ai_imc_stack_fault_saved_sstatus\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_sfr_restore_epc\n"
    "  csrw sstatus, t4\n"
    ".Lai_imc_sfr_restore_epc:\n"
    "  csrw sepc, t1\n"
    ".Lai_imc_sfr_restore_regs:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 96\n"
    "  sret\n");

asm(
    ".align 2\n"
    ".global ai_imc_stvec_nested_fault_preserve_retry_handler\n"
    "ai_imc_stvec_nested_fault_preserve_retry_handler:\n"
    "  addi sp, sp, -96\n"
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
    "  csrr t3, sip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_nfp_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t4, 0x696d636e66700000\n"
    "  add t4, t4, t5\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  slli t3, t5, 3\n"
    "  add t6, t6, t3\n"
    "  sd t4, 0(t6)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_nfp_log_done:\n"
    "  srli t3, t0, 63\n"
    "  bnez t3, .Lai_imc_nfp_interrupt\n"
    "  li t3, 15\n"
    "  bne t0, t3, .Lai_imc_nfp_restore_regs\n"
    "  la t3, ai_imc_nested_fault_in_handler\n"
    "  ld t4, 0(t3)\n"
    "  bnez t4, .Lai_imc_nfp_handler_fault\n"
    "  csrr t6, sstatus\n"
    "  la t3, ai_imc_nested_fault_main_saved_sstatus\n"
    "  sd t6, 0(t3)\n"
    "  li t6, 2\n"
    "  csrc sie, t6\n"
    "  csrc sip, t6\n"
    "  csrs sip, t6\n"
    "  csrs sie, t6\n"
    "  csrs sstatus, t6\n"
    "  li t5, 4096\n"
    ".Lai_imc_nfp_wait_ssip:\n"
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t6, 3\n"
    "  bgeu t4, t6, .Lai_imc_nfp_wait_done\n"
    "  addi t5, t5, -1\n"
    "  bnez t5, .Lai_imc_nfp_wait_ssip\n"
    ".Lai_imc_nfp_wait_done:\n"
    "  li t6, 2\n"
    "  csrc sstatus, t6\n"
    "  la t3, ai_imc_nested_fault_main_repairs\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t3, ai_imc_nested_fault_main_pte_addr\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, .Lai_imc_nfp_restore_main_status\n"
    "  la t3, ai_imc_nested_fault_main_pte_value\n"
    "  ld t6, 0(t3)\n"
    "  sd t6, 0(t4)\n"
    "  sfence.vma\n"
    ".Lai_imc_nfp_restore_main_status:\n"
    "  la t3, ai_imc_nested_fault_main_saved_sstatus\n"
    "  ld t6, 0(t3)\n"
    "  beqz t6, .Lai_imc_nfp_restore_main_epc\n"
    "  csrw sstatus, t6\n"
    ".Lai_imc_nfp_restore_main_epc:\n"
    "  csrw sepc, t1\n"
    "  j .Lai_imc_nfp_restore_regs\n"
    ".Lai_imc_nfp_handler_fault:\n"
    "  la t3, ai_imc_nested_fault_handler_repairs\n"
    "  ld t4, 0(t3)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "  la t3, ai_imc_nested_fault_handler_pte_addr\n"
    "  ld t4, 0(t3)\n"
    "  beqz t4, .Lai_imc_nfp_restore_regs\n"
    "  la t3, ai_imc_nested_fault_handler_pte_value\n"
    "  ld t6, 0(t3)\n"
    "  sd t6, 0(t4)\n"
    "  sfence.vma\n"
    "  j .Lai_imc_nfp_restore_regs\n"
    ".Lai_imc_nfp_interrupt:\n"
    "  andi t3, t0, 63\n"
    "  li t4, 1\n"
    "  bne t3, t4, .Lai_imc_nfp_clear_interrupt\n"
    "  csrr t6, sstatus\n"
    "  la t4, ai_imc_nested_fault_ssip_saved_sstatus\n"
    "  sd t6, 0(t4)\n"
    "  la t4, ai_imc_nested_fault_in_handler\n"
    "  li t6, 1\n"
    "  sd t6, 0(t4)\n"
    "  la t4, ai_imc_nested_fault_handler_store_addr\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_nfp_handler_store_done\n"
    "  la t6, ai_imc_nested_fault_handler_store_value\n"
    "  ld t6, 0(t6)\n"
    "  sd t6, 0(t4)\n"
    "  fence rw, rw\n"
    ".Lai_imc_nfp_handler_store_done:\n"
    "  la t4, ai_imc_nested_fault_in_handler\n"
    "  sd zero, 0(t4)\n"
    ".Lai_imc_nfp_clear_interrupt:\n"
    "  andi t3, t0, 63\n"
    "  li t4, 1\n"
    "  sll t4, t4, t3\n"
    "  csrc sip, t4\n"
    "  la t4, ai_imc_nested_fault_ssip_saved_sstatus\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_nfp_restore_epc\n"
    "  csrw sstatus, t4\n"
    ".Lai_imc_nfp_restore_epc:\n"
    "  csrw sepc, t1\n"
    ".Lai_imc_nfp_restore_regs:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 96\n"
    "  sret\n");

asm(
    ".align 2\n"
    ".global ai_imc_stvec_same_target_load_handler\n"
    "ai_imc_stvec_same_target_load_handler:\n"
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
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, sip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_imc_handler_probe_addr\n"
    "  ld t2, 0(t6)\n"
    "  beqz t2, 1f\n"
    "  ld t2, 0(t2)\n"
    "  j 3f\n"
    "1:\n"
    "  li t2, 0\n"
    "3:\n"
    "  la t6, ai_imc_handler_probe_value\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x696d6373686c6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  srli t1, t0, 63\n"
    "  beqz t1, 4f\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc sip, t2\n"
    "4:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  sret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_retry_store_handler\n"
    "ai_imc_mtvec_retry_store_handler:\n"
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
    "  csrr t3, mip\n"
    "  li t4, 0x20000\n"
    "  csrc mstatus, t4\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_retry_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_retry_log_done:\n"
    "  li t4, 7\n"
    "  beq t0, t4, .Lai_imc_retry_repair_store_fault\n"
    "  srli t4, t0, 63\n"
    "  beqz t4, .Lai_imc_retry_exit_plain\n"
    "  andi t4, t0, 63\n"
    "  li t5, 5\n"
    "  bne t4, t5, .Lai_imc_retry_exit_plain\n"
    "  li t5, 1\n"
    "  sll t5, t5, t4\n"
    "  csrc mip, t5\n"
    "  j .Lai_imc_retry_exit_plain\n"
    ".Lai_imc_retry_repair_store_fault:\n"
    "  la t4, ai_imc_retry_fault_repairs\n"
    "  ld t5, 0(t4)\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    "  csrw pmpcfg0, zero\n"
    "  la t4, ai_imc_retry_pmpaddr0\n"
    "  ld t5, 0(t4)\n"
    "  csrw pmpaddr0, t5\n"
    "  la t4, ai_imc_retry_pmpaddr1\n"
    "  ld t5, 0(t4)\n"
    "  csrw pmpaddr1, t5\n"
    "  la t4, ai_imc_retry_pmpaddr2\n"
    "  ld t5, 0(t4)\n"
    "  csrw pmpaddr2, t5\n"
    "  li t5, 0x000f0f0f\n"
    "  csrw pmpcfg0, t5\n"
    "  fence rw, rw\n"
    "  li t4, 0x1800\n"
    "  csrs mstatus, t4\n"
    "  li t4, 0x20000\n"
    "  csrs mstatus, t4\n"
    "  j .Lai_imc_retry_restore\n"
    ".Lai_imc_retry_exit_plain:\n"
    "  li t4, 0x20000\n"
    "  csrc mstatus, t4\n"
    ".Lai_imc_retry_restore:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_same_target_load_handler\n"
    "ai_imc_mtvec_same_target_load_handler:\n"
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
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_imc_handler_probe_addr\n"
    "  ld t2, 0(t6)\n"
    "  beqz t2, 1f\n"
    "  ld t2, 0(t2)\n"
    "  j 3f\n"
    "1:\n"
    "  li t2, 0\n"
    "3:\n"
    "  la t6, ai_imc_handler_probe_value\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_partial_store_handler\n"
    "ai_imc_mtvec_partial_store_handler:\n"
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
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_imc_handler_store_addr\n"
    "  ld t1, 0(t6)\n"
    "  beqz t1, 1f\n"
    "  la t6, ai_imc_handler_store_value\n"
    "  ld t2, 0(t6)\n"
    "  sb t2, 0(t1)\n"
    "  fence rw, rw\n"
    "1:\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mprv_store_retry_probe\n"
    "ai_imc_mprv_store_retry_probe:\n"
    "  la t0, .Lai_imc_mprv_retry_store\n"
    "  la t1, ai_imc_retry_store_pc\n"
    "  sd t0, 0(t1)\n"
    "  la t0, .Lai_imc_mprv_retry_after\n"
    "  la t1, ai_imc_retry_after_pc\n"
    "  sd t0, 0(t1)\n"
    "  li t0, 0x21800\n"
    "  csrc mstatus, t0\n"
    "  li t0, 0x20000\n"
    "  csrs mstatus, t0\n"
    ".Lai_imc_mprv_retry_store:\n"
    "  sd a1, 0(a0)\n"
    ".Lai_imc_mprv_retry_after:\n"
    "  li t0, 0x20000\n"
    "  csrc mstatus, t0\n"
    "  ret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_mprv_save_handler\n"
    "ai_imc_mtvec_mprv_save_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  sd t0, 56(sp)\n"
    "  csrr t0, mepc\n"
    "  sd t0, 64(sp)\n"
    "  csrr t0, mtval\n"
    "  sd t0, 72(sp)\n"
    "  csrr t0, mstatus\n"
    "  sd t0, 80(sp)\n"
    "  csrr t0, mip\n"
    "  sd t0, 88(sp)\n"
    "  li t0, 0x20000\n"
    "  csrc mstatus, t0\n"
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  ld t0, 56(sp)\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  ld t0, 64(sp)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  ld t0, 72(sp)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  ld t0, 88(sp)\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  ld t0, 80(sp)\n"
    "  la t6, ai_imc_handler_mstatus\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_handler_probe_addr\n"
    "  ld t0, 0(t6)\n"
    "  beqz t0, 1f\n"
    "  ld t0, 0(t0)\n"
    "  j 3f\n"
    "1:\n"
    "  li t0, 0\n"
    "3:\n"
    "  la t6, ai_imc_handler_probe_value\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  li t0, 0x696d63686c6f6700\n"
    "  add t0, t0, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  ld t0, 56(sp)\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 80(sp)\n"
    "  csrw mstatus, t0\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_amo_log_handler\n"
    "ai_imc_mtvec_amo_log_handler:\n"
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
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 2f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_imc_handler_probe_addr\n"
    "  ld t1, 0(t6)\n"
    "  beqz t1, 1f\n"
    "  la t6, ai_imc_handler_amo_addend\n"
    "  ld t2, 0(t6)\n"
    "  amoadd.d t2, t2, (t1)\n"
    "  la t6, ai_imc_handler_amo_old\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  ld t1, 0(t1)\n"
    "  j 3f\n"
    "1:\n"
    "  li t2, 0\n"
    "  li t1, 0\n"
    "3:\n"
    "  la t6, ai_imc_handler_probe_value\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_handler_amo_final\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "2:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_pma_atomic_retry_generation_handler\n"
    "ai_imc_mtvec_pma_atomic_retry_generation_handler:\n"
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
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_pma_retry_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  li t2, 0x696d63706d616700\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_pma_retry_log_done:\n"
    "  li t4, 7\n"
    "  beq t0, t4, .Lai_imc_pma_retry_repair_saf\n"
    "  srli t4, t0, 63\n"
    "  beqz t4, .Lai_imc_pma_retry_restore\n"
    "  andi t4, t0, 63\n"
    "  li t5, 5\n"
    "  bne t4, t5, .Lai_imc_pma_retry_restore\n"
    "  la t5, ai_imc_pma_retry_interrupts\n"
    "  ld t6, 0(t5)\n"
    "  addi t6, t6, 1\n"
    "  sd t6, 0(t5)\n"
    "  csrr t6, 0x7c0\n"
    "  la t5, ai_imc_pma_retry_pmacfg_seen_in_interrupt\n"
    "  sd t6, 0(t5)\n"
    "  la t5, ai_imc_pma_retry_target_addr\n"
    "  ld t5, 0(t5)\n"
    "  beqz t5, .Lai_imc_pma_retry_clear_stip\n"
    "  ld t6, 0(t5)\n"
    "  la t5, ai_imc_pma_retry_target_after_complete\n"
    "  sd t6, 0(t5)\n"
    ".Lai_imc_pma_retry_clear_stip:\n"
    "  li t5, 0x20\n"
    "  csrc mip, t5\n"
    "  j .Lai_imc_pma_retry_restore\n"
    ".Lai_imc_pma_retry_repair_saf:\n"
    "  la t4, ai_imc_pma_retry_fault_repairs\n"
    "  ld t5, 0(t4)\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    "  la t4, ai_imc_pma_retry_target_addr\n"
    "  ld t5, 0(t4)\n"
    "  beqz t5, .Lai_imc_pma_retry_program_allow\n"
    "  ld t6, 0(t5)\n"
    "  la t4, ai_imc_pma_retry_value_on_fault\n"
    "  sd t6, 0(t4)\n"
    ".Lai_imc_pma_retry_program_allow:\n"
    "  csrw 0x7c0, zero\n"
    "  la t4, ai_imc_pma_retry_pmaaddr0\n"
    "  ld t5, 0(t4)\n"
    "  csrw 0x7c8, t5\n"
    "  la t4, ai_imc_pma_retry_pmaaddr1\n"
    "  ld t5, 0(t4)\n"
    "  csrw 0x7c9, t5\n"
    "  la t4, ai_imc_pma_retry_allow_cfg\n"
    "  ld t5, 0(t4)\n"
    "  csrw 0x7c0, t5\n"
    "  fence rw, rw\n"
    "  li t5, 0x20\n"
    "  csrs mip, t5\n"
    "  li t5, 0x80\n"
    "  csrs mstatus, t5\n"
    ".Lai_imc_pma_retry_restore:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_clint_msip_clear_handler\n"
    "ai_imc_mtvec_clint_msip_clear_handler:\n"
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
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_clint_msip_direct_clear\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  la t4, ai_imc_clint_msip_addr\n"
    "  ld t4, 0(t4)\n"
    "  li t1, 0\n"
    "  beqz t4, .Lai_imc_clint_msip_log_preclear\n"
    "  lwu t1, 0(t4)\n"
    ".Lai_imc_clint_msip_log_preclear:\n"
    "  la t2, ai_imc_clint_msip_before_clear\n"
    "  add t2, t2, t6\n"
    "  sd t1, 0(t2)\n"
    "  la t2, ai_imc_clint_handler_log_addr\n"
    "  ld t2, 0(t2)\n"
    "  li t1, 0\n"
    "  beqz t2, .Lai_imc_clint_msip_log_dram\n"
    "  ld t1, 0(t2)\n"
    ".Lai_imc_clint_msip_log_dram:\n"
    "  la t2, ai_imc_clint_log_value\n"
    "  add t2, t2, t6\n"
    "  sd t1, 0(t2)\n"
    "  li t1, 0x696d63636c6e7400\n"
    "  add t1, t1, t5\n"
    "  la t2, ai_imc_handler_log_page\n"
    "  add t2, t2, t6\n"
    "  sd t1, 0(t2)\n"
    "  beqz t4, .Lai_imc_clint_msip_log_after\n"
    "  sw zero, 0(t4)\n"
    "  fence rw, rw\n"
    "  lwu t1, 0(t4)\n"
    "  j .Lai_imc_clint_msip_store_after\n"
    ".Lai_imc_clint_msip_log_after:\n"
    "  li t1, 0\n"
    ".Lai_imc_clint_msip_store_after:\n"
    "  la t2, ai_imc_clint_msip_after_clear\n"
    "  add t2, t2, t6\n"
    "  sd t1, 0(t2)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    "  j .Lai_imc_clint_msip_restore\n"
    ".Lai_imc_clint_msip_direct_clear:\n"
    "  la t4, ai_imc_clint_msip_addr\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_clint_msip_restore\n"
    "  sw zero, 0(t4)\n"
    "  fence rw, rw\n"
    ".Lai_imc_clint_msip_restore:\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_spill_stack_handler\n"
    "ai_imc_mtvec_spill_stack_handler:\n"
    "  mv t6, sp\n"
    "  la sp, ai_imc_handler_spill_stack\n"
    "  addi sp, sp, 128\n"
    "  addi sp, sp, -64\n"
    "  sd t6, 0(sp)\n"
    "  sd t0, 8(sp)\n"
    "  sd t1, 16(sp)\n"
    "  sd t2, 24(sp)\n"
    "  sd t3, 32(sp)\n"
    "  sd t4, 40(sp)\n"
    "  sd t5, 48(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  la t3, ai_imc_trap_count\n"
    "  ld t4, 0(t3)\n"
    "  li t5, 4\n"
    "  bgeu t4, t5, 1f\n"
    "  slli t5, t4, 3\n"
    "  la t6, ai_imc_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imc_trap_epc\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imc_trap_tval\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  csrr t2, mip\n"
    "  la t6, ai_imc_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  ld t2, 0(sp)\n"
    "  la t6, ai_imc_handler_saved_sp\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  slli t5, t4, 5\n"
    "  la t6, ai_imc_handler_spill_stack\n"
    "  add t6, t6, t5\n"
    "  li t2, 0x696d6373706c0000\n"
    "  add t2, t2, t4\n"
    "  sd t2, 0(t6)\n"
    "  addi t2, t2, 0x11\n"
    "  sd t2, 8(t6)\n"
    "  addi t2, t2, 0x11\n"
    "  sd t2, 16(t6)\n"
    "  addi t2, t2, 0x11\n"
    "  sd t2, 24(t6)\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t4\n"
    "  la t6, ai_imc_handler_log_page\n"
    "  slli t5, t4, 3\n"
    "  add t6, t6, t5\n"
    "  sd t2, 0(t6)\n"
    "  addi t4, t4, 1\n"
    "  sd t4, 0(t3)\n"
    "1:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t6, 0(sp)\n"
    "  ld t0, 8(sp)\n"
    "  ld t1, 16(sp)\n"
    "  ld t2, 24(sp)\n"
    "  ld t3, 32(sp)\n"
    "  ld t4, 40(sp)\n"
    "  ld t5, 48(sp)\n"
    "  mv sp, t6\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_time_log_handler\n"
    "ai_imc_mtvec_time_log_handler:\n"
    "  addi sp, sp, -80\n"
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
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_time_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  csrr t2, 0xc01\n"
    "  la t4, ai_imc_time_log_value\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_time_log_store_addr\n"
    "  ld t4, 0(t4)\n"
    "  beqz t4, .Lai_imc_time_skip_store\n"
    "  sd t2, 0(t4)\n"
    ".Lai_imc_time_skip_store:\n"
    "  li t2, 0x696d63686c6f6700\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_time_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 80\n"
    "  mret\n"
    "\n"
    ".align 2\n"
    ".global ai_imc_mtvec_message_ack_handler\n"
    "ai_imc_mtvec_message_ack_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  sd a0, 56(sp)\n"
    "  sd a1, 64(sp)\n"
    "  sd a2, 72(sp)\n"
    "  sd a3, 80(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_msg_done\n"
    "  slli t6, t5, 3\n"
    "  la a0, ai_imc_trap_cause\n"
    "  add a0, a0, t6\n"
    "  sd t0, 0(a0)\n"
    "  la a0, ai_imc_trap_epc\n"
    "  add a0, a0, t6\n"
    "  sd t1, 0(a0)\n"
    "  la a0, ai_imc_trap_tval\n"
    "  add a0, a0, t6\n"
    "  sd t2, 0(a0)\n"
    "  la a0, ai_imc_trap_mip\n"
    "  add a0, a0, t6\n"
    "  sd t3, 0(a0)\n"
    "  fence r, rw\n"
    "  la a0, ai_imc_message_seq_addr\n"
    "  ld a0, 0(a0)\n"
    "  ld a1, 0(a0)\n"
    "  la a0, ai_imc_message_payload_addr\n"
    "  ld a0, 0(a0)\n"
    "  ld a2, 0(a0)\n"
    "  la a0, ai_imc_message_checksum_addr\n"
    "  ld a0, 0(a0)\n"
    "  ld a3, 0(a0)\n"
    "  la a0, ai_imc_message_seq_log\n"
    "  add a0, a0, t6\n"
    "  sd a1, 0(a0)\n"
    "  la a0, ai_imc_message_payload_log\n"
    "  add a0, a0, t6\n"
    "  sd a2, 0(a0)\n"
    "  la a0, ai_imc_message_checksum_log\n"
    "  add a0, a0, t6\n"
    "  sd a3, 0(a0)\n"
    "  la a0, ai_imc_message_ack_addr\n"
    "  ld a0, 0(a0)\n"
    "  sd a1, 0(a0)\n"
    "  fence rw, rw\n"
    "  la a0, ai_imc_message_ack_log\n"
    "  add a0, a0, t6\n"
    "  sd a1, 0(a0)\n"
    "  li a0, 0x696d63686c6f6700\n"
    "  add a0, a0, t5\n"
    "  la a1, ai_imc_handler_log_page\n"
    "  add a1, a1, t6\n"
    "  sd a0, 0(a1)\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_msg_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_cbozero_once\n"
    "ai_imc_cbozero_once:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  .insn i 0x0f, 0x2, x0, a0, 4\n"
    "  fence rw, rw\n"
    "  ret\n"
    "  .option pop\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_cbozero_handler\n"
    "ai_imc_mtvec_cbozero_handler:\n"
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
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_cbo_log_done\n"
    "  slli t6, t5, 3\n"
    "  la t4, ai_imc_trap_cause\n"
    "  add t4, t4, t6\n"
    "  sd t0, 0(t4)\n"
    "  la t4, ai_imc_trap_epc\n"
    "  add t4, t4, t6\n"
    "  sd t1, 0(t4)\n"
    "  la t4, ai_imc_trap_tval\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_mip\n"
    "  add t4, t4, t6\n"
    "  sd t3, 0(t4)\n"
    "  la t4, ai_imc_handler_cbo_addr\n"
    "  ld t1, 0(t4)\n"
    "  beqz t1, .Lai_imc_cbo_skip\n"
    "  .option push\n"
    "  .option norvc\n"
    "  .insn i 0x0f, 0x2, x0, t1, 4\n"
    "  .option pop\n"
    "  fence rw, rw\n"
    ".Lai_imc_cbo_skip:\n"
    "  li t2, 0x696d636863626f00\n"
    "  add t2, t2, t5\n"
    "  la t4, ai_imc_handler_log_page\n"
    "  add t4, t4, t6\n"
    "  sd t2, 0(t4)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_cbo_log_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  addi sp, sp, 64\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_vector_context_handler\n"
    "ai_imc_mtvec_vector_context_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  sd a0, 56(sp)\n"
    "  sd a1, 64(sp)\n"
    "  sd a2, 72(sp)\n"
    "  sd a3, 80(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_vec_ctx_done\n"
    "  slli t6, t5, 3\n"
    "  la a0, ai_imc_trap_cause\n"
    "  add a0, a0, t6\n"
    "  sd t0, 0(a0)\n"
    "  la a0, ai_imc_trap_epc\n"
    "  add a0, a0, t6\n"
    "  sd t1, 0(a0)\n"
    "  la a0, ai_imc_trap_tval\n"
    "  add a0, a0, t6\n"
    "  sd t2, 0(a0)\n"
    "  la a0, ai_imc_trap_mip\n"
    "  add a0, a0, t6\n"
    "  sd t3, 0(a0)\n"
    "  csrr a0, vl\n"
    "  csrr a1, vtype\n"
    "  csrr a2, vstart\n"
    "  la a3, ai_imc_handler_vec_saved_vl\n"
    "  add a3, a3, t6\n"
    "  sd a0, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vtype\n"
    "  add a3, a3, t6\n"
    "  sd a1, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vstart\n"
    "  add a3, a3, t6\n"
    "  sd a2, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_save_addr\n"
    "  ld a3, 0(a3)\n"
    "  beqz a3, .Lai_imc_vec_ctx_skip_save\n"
    "  .option push\n"
    "  .option norvc\n"
    "  vsetvli t1, a0, e32, m1, tu, mu\n"
    "  csrw vstart, zero\n"
    "  vse32.v v6, (a3)\n"
    "  .option pop\n"
    ".Lai_imc_vec_ctx_skip_save:\n"
    "  la a3, ai_imc_handler_vec_src_addr\n"
    "  ld a3, 0(a3)\n"
    "  la t1, ai_imc_handler_vec_dst_addr\n"
    "  ld t1, 0(t1)\n"
    "  beqz a3, .Lai_imc_vec_ctx_skip_handler_vec\n"
    "  beqz t1, .Lai_imc_vec_ctx_skip_handler_vec\n"
    "  .option push\n"
    "  .option norvc\n"
    "  vsetvli t2, zero, e16, m1, tu, mu\n"
    "  vle16.v v8, (a3)\n"
    "  vse16.v v8, (t1)\n"
    "  .option pop\n"
    "  csrr t2, vl\n"
    "  csrr t3, vtype\n"
    "  la a3, ai_imc_handler_vec_handler_vl\n"
    "  add a3, a3, t6\n"
    "  sd t2, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_handler_vtype\n"
    "  add a3, a3, t6\n"
    "  sd t3, 0(a3)\n"
    ".Lai_imc_vec_ctx_skip_handler_vec:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  csrw vstart, zero\n"
    "  vsetvl t1, a0, a1\n"
    "  .option pop\n"
    "  li a0, 0x696d637676637400\n"
    "  add a0, a0, t5\n"
    "  la a1, ai_imc_handler_log_page\n"
    "  add a1, a1, t6\n"
    "  sd a0, 0(a1)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_vec_ctx_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_mask_context_handler\n"
    "ai_imc_mtvec_mask_context_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  sd a0, 56(sp)\n"
    "  sd a1, 64(sp)\n"
    "  sd a2, 72(sp)\n"
    "  sd a3, 80(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_mask_ctx_done\n"
    "  slli t6, t5, 3\n"
    "  la a0, ai_imc_trap_cause\n"
    "  add a0, a0, t6\n"
    "  sd t0, 0(a0)\n"
    "  la a0, ai_imc_trap_epc\n"
    "  add a0, a0, t6\n"
    "  sd t1, 0(a0)\n"
    "  la a0, ai_imc_trap_tval\n"
    "  add a0, a0, t6\n"
    "  sd t2, 0(a0)\n"
    "  la a0, ai_imc_trap_mip\n"
    "  add a0, a0, t6\n"
    "  sd t3, 0(a0)\n"
    "  csrr a0, vl\n"
    "  csrr a1, vtype\n"
    "  csrr a2, vstart\n"
    "  la a3, ai_imc_handler_vec_saved_vl\n"
    "  add a3, a3, t6\n"
    "  sd a0, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vtype\n"
    "  add a3, a3, t6\n"
    "  sd a1, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vstart\n"
    "  add a3, a3, t6\n"
    "  sd a2, 0(a3)\n"
    "  la a3, ai_imc_handler_mask_save_addr\n"
    "  ld a3, 0(a3)\n"
    "  beqz a3, .Lai_imc_mask_ctx_skip_save\n"
    "  .option push\n"
    "  .option norvc\n"
    "  csrw vstart, zero\n"
    "  vsm.v v0, (a3)\n"
    "  .option pop\n"
    ".Lai_imc_mask_ctx_skip_save:\n"
    "  la a3, ai_imc_handler_mask_src_addr\n"
    "  ld a3, 0(a3)\n"
    "  la t1, ai_imc_handler_mask_dump_addr\n"
    "  ld t1, 0(t1)\n"
    "  la t2, ai_imc_handler_mask_payload_addr\n"
    "  ld t2, 0(t2)\n"
    "  la t3, ai_imc_handler_mask_target_addr\n"
    "  ld t3, 0(t3)\n"
    "  beqz a3, .Lai_imc_mask_ctx_skip_handler_mask\n"
    "  beqz t1, .Lai_imc_mask_ctx_skip_handler_mask\n"
    "  beqz t2, .Lai_imc_mask_ctx_skip_handler_mask\n"
    "  beqz t3, .Lai_imc_mask_ctx_skip_handler_mask\n"
    "  .option push\n"
    "  .option norvc\n"
    "  li t4, 9\n"
    "  vsetvli t4, t4, e16, m2, tu, mu\n"
    "  csrw vstart, zero\n"
    "  vlm.v v0, (a3)\n"
    "  vsm.v v0, (t1)\n"
    "  vle16.v v8, (t2)\n"
    "  vse16.v v8, (t3), v0.t\n"
    "  .option pop\n"
    "  csrr t4, vl\n"
    "  csrr t3, vtype\n"
    "  la a3, ai_imc_handler_vec_handler_vl\n"
    "  add a3, a3, t6\n"
    "  sd t4, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_handler_vtype\n"
    "  add a3, a3, t6\n"
    "  sd t3, 0(a3)\n"
    ".Lai_imc_mask_ctx_skip_handler_mask:\n"
    "  la a3, ai_imc_handler_mask_save_addr\n"
    "  ld a3, 0(a3)\n"
    "  beqz a3, .Lai_imc_mask_ctx_skip_restore\n"
    "  .option push\n"
    "  .option norvc\n"
    "  vsetvl t1, a0, a1\n"
    "  csrw vstart, zero\n"
    "  vlm.v v0, (a3)\n"
    "  .option pop\n"
    ".Lai_imc_mask_ctx_skip_restore:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  csrw vstart, zero\n"
    "  vsetvl t1, a0, a1\n"
    "  .option pop\n"
    "  li a0, 0x696d63766d736b00\n"
    "  add a0, a0, t5\n"
    "  la a1, ai_imc_handler_log_page\n"
    "  add a1, a1, t6\n"
    "  sd a0, 0(a1)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_mask_ctx_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n");

asm(
    ".align 2\n"
    ".global ai_imc_mtvec_segment_spill_handler\n"
    "ai_imc_mtvec_segment_spill_handler:\n"
    "  addi sp, sp, -96\n"
    "  sd t0, 0(sp)\n"
    "  sd t1, 8(sp)\n"
    "  sd t2, 16(sp)\n"
    "  sd t3, 24(sp)\n"
    "  sd t4, 32(sp)\n"
    "  sd t5, 40(sp)\n"
    "  sd t6, 48(sp)\n"
    "  sd a0, 56(sp)\n"
    "  sd a1, 64(sp)\n"
    "  sd a2, 72(sp)\n"
    "  sd a3, 80(sp)\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mepc\n"
    "  csrr t2, mtval\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imc_trap_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imc_seg_spill_done\n"
    "  slli t6, t5, 3\n"
    "  la a0, ai_imc_trap_cause\n"
    "  add a0, a0, t6\n"
    "  sd t0, 0(a0)\n"
    "  la a0, ai_imc_trap_epc\n"
    "  add a0, a0, t6\n"
    "  sd t1, 0(a0)\n"
    "  la a0, ai_imc_trap_tval\n"
    "  add a0, a0, t6\n"
    "  sd t2, 0(a0)\n"
    "  la a0, ai_imc_trap_mip\n"
    "  add a0, a0, t6\n"
    "  sd t3, 0(a0)\n"
    "  csrr a0, vl\n"
    "  csrr a1, vtype\n"
    "  csrr a2, vstart\n"
    "  la a3, ai_imc_handler_vec_saved_vl\n"
    "  add a3, a3, t6\n"
    "  sd a0, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vtype\n"
    "  add a3, a3, t6\n"
    "  sd a1, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_saved_vstart\n"
    "  add a3, a3, t6\n"
    "  sd a2, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_save_addr\n"
    "  ld a3, 0(a3)\n"
    "  beqz a3, .Lai_imc_seg_spill_skip_save\n"
    "  .option push\n"
    "  .option norvc\n"
    "  vsetvli t1, zero, e32, m1, tu, mu\n"
    "  csrw vstart, zero\n"
    "  vsseg2e32.v v8, (a3)\n"
    "  .option pop\n"
    ".Lai_imc_seg_spill_skip_save:\n"
    "  la a3, ai_imc_handler_vec_src_addr\n"
    "  ld a3, 0(a3)\n"
    "  la t1, ai_imc_handler_vec_dst_addr\n"
    "  ld t1, 0(t1)\n"
    "  beqz a3, .Lai_imc_seg_spill_skip_handler_vec\n"
    "  beqz t1, .Lai_imc_seg_spill_skip_handler_vec\n"
    "  .option push\n"
    "  .option norvc\n"
    "  vsetvli t2, zero, e16, m1, tu, mu\n"
    "  vle16.v v12, (a3)\n"
    "  vse16.v v12, (t1)\n"
    "  .option pop\n"
    "  csrr t2, vl\n"
    "  csrr t3, vtype\n"
    "  la a3, ai_imc_handler_vec_handler_vl\n"
    "  add a3, a3, t6\n"
    "  sd t2, 0(a3)\n"
    "  la a3, ai_imc_handler_vec_handler_vtype\n"
    "  add a3, a3, t6\n"
    "  sd t3, 0(a3)\n"
    ".Lai_imc_seg_spill_skip_handler_vec:\n"
    "  .option push\n"
    "  .option norvc\n"
    "  csrw vstart, zero\n"
    "  vsetvl t1, a0, a1\n"
    "  .option pop\n"
    "  li a0, 0x696d637673706900\n"
    "  add a0, a0, t5\n"
    "  la a1, ai_imc_handler_log_page\n"
    "  add a1, a1, t6\n"
    "  sd a0, 0(a1)\n"
    "  la t4, ai_imc_trap_count\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imc_seg_spill_done:\n"
    "  andi t1, t0, 63\n"
    "  li t2, 1\n"
    "  sll t2, t2, t1\n"
    "  csrc mip, t2\n"
    "  ld t0, 0(sp)\n"
    "  ld t1, 8(sp)\n"
    "  ld t2, 16(sp)\n"
    "  ld t3, 24(sp)\n"
    "  ld t4, 32(sp)\n"
    "  ld t5, 40(sp)\n"
    "  ld t6, 48(sp)\n"
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  mret\n");

static inline void ai_imc_prepare_stip(uint64_t saved_mideleg,
    uint64_t enable)
{
    (void)saved_mideleg;
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MENVCFG, MENVCFG_STCE);
    CSRC(CSR_MIE, AI_IMC_MIE_STIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRC(CSR_MIDELEG, AI_IMC_MIP_STIP);
    if (enable) {
        CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    }
}

static inline void ai_imc_prepare_msip(uint64_t enable)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IMC_MIE_MSIE);
    CSRC(CSR_MIP, AI_IMC_MIP_MSIP);
    CSRC(CSR_MIDELEG, AI_IMC_MIP_MSIP);
    if (enable) {
        CSRS(CSR_MIE, AI_IMC_MIE_MSIE);
    }
}

static inline uint64_t ai_imc_read_spfctl(void)
{
    return CSRR(CSR_SPFCTL);
}

static inline void ai_imc_write_spfctl(uint64_t value)
{
    CSRW(CSR_SPFCTL, value);
}

static inline void ai_imc_prepare_hs_ssip(uint64_t enable)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    CSRS(CSR_MIDELEG, AI_IMC_SIP_SSIP);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    if (enable) {
        CSRS(CSR_SIE, AI_IMC_SIE_SSIE);
    }
}

static inline void ai_imc_restore_m_interrupt_state(uint64_t saved_mstatus,
    uint64_t saved_mie, uint64_t saved_mideleg)
{
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP | AI_IMC_MIP_MSIP |
        AI_IMC_SIP_SSIP);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_imc_restore_s_interrupt_state(uint64_t saved_sstatus,
    uint64_t saved_sie)
{
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_SSTATUS, saved_sstatus);
}

static inline uint64_t ai_imc_spin_until_trap(uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && !excpt.triggered; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline uint64_t ai_imc_spin_until_log_count(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && ai_imc_trap_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline void ai_imc_reset_trap_log(void)
{
    ai_imc_trap_count = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        ai_imc_trap_cause[i] = 0ULL;
        ai_imc_trap_epc[i] = 0ULL;
        ai_imc_trap_tval[i] = 0ULL;
        ai_imc_trap_mip[i] = 0ULL;
        ai_imc_handler_log_page[i] = 0ULL;
        ai_imc_handler_probe_value[i] = 0ULL;
        ai_imc_same_pc_helper_pc[i] = 0ULL;
        ai_imc_handler_mstatus[i] = 0ULL;
        ai_imc_handler_amo_old[i] = 0ULL;
        ai_imc_handler_amo_final[i] = 0ULL;
        ai_imc_handler_saved_sp[i] = 0ULL;
        ai_imc_message_seq_log[i] = 0ULL;
        ai_imc_message_payload_log[i] = 0ULL;
        ai_imc_message_checksum_log[i] = 0ULL;
        ai_imc_message_ack_log[i] = 0ULL;
        ai_imc_time_log_value[i] = 0ULL;
        ai_imc_handler_vec_saved_vl[i] = 0ULL;
        ai_imc_handler_vec_saved_vtype[i] = 0ULL;
        ai_imc_handler_vec_saved_vstart[i] = 0ULL;
        ai_imc_handler_vec_handler_vl[i] = 0ULL;
        ai_imc_handler_vec_handler_vtype[i] = 0ULL;
        for (uint64_t j = 0ULL; j < 4ULL; ++j) {
            ai_imc_handler_spill_stack[i * 4ULL + j] = 0ULL;
        }
    }
    ai_imc_handler_probe_addr = 0ULL;
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    ai_imc_handler_amo_addend = 0ULL;
    ai_imc_handler_spill_guard0 = 0ULL;
    ai_imc_handler_spill_guard1 = 0ULL;
    ai_imc_handler_cbo_addr = 0ULL;
    ai_imc_retry_store_pc = 0ULL;
    ai_imc_retry_after_pc = 0ULL;
    ai_imc_retry_fault_repairs = 0ULL;
    ai_imc_retry_pmpaddr0 = 0ULL;
    ai_imc_retry_pmpaddr1 = 0ULL;
    ai_imc_retry_pmpaddr2 = 0ULL;
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr = 0ULL;
    ai_imc_block_copy_pte_value = 0ULL;
    ai_imc_block_copy_restore_mtvec = 0ULL;
    ai_imc_block_copy_repairs = 0ULL;
    ai_imc_message_seq_addr = 0ULL;
    ai_imc_message_payload_addr = 0ULL;
    ai_imc_message_checksum_addr = 0ULL;
    ai_imc_message_ack_addr = 0ULL;
    ai_imc_time_log_store_addr = 0ULL;
    ai_imc_handler_vec_src_addr = 0ULL;
    ai_imc_handler_vec_dst_addr = 0ULL;
    ai_imc_handler_vec_save_addr = 0ULL;
    ai_imc_handler_mask_save_addr = 0ULL;
    ai_imc_handler_mask_src_addr = 0ULL;
    ai_imc_handler_mask_dump_addr = 0ULL;
    ai_imc_handler_mask_payload_addr = 0ULL;
    ai_imc_handler_mask_target_addr = 0ULL;
    ai_imc_pma_retry_target_addr = 0ULL;
    ai_imc_pma_retry_pmaaddr0 = 0ULL;
    ai_imc_pma_retry_pmaaddr1 = 0ULL;
    ai_imc_pma_retry_allow_cfg = 0ULL;
    ai_imc_pma_retry_fault_repairs = 0ULL;
    ai_imc_pma_retry_interrupts = 0ULL;
    ai_imc_pma_retry_value_on_fault = 0ULL;
    ai_imc_pma_retry_pmacfg_seen_in_interrupt = 0ULL;
    ai_imc_pma_retry_target_after_complete = 0ULL;
    ai_imc_clint_msip_addr = 0ULL;
    ai_imc_clint_handler_log_addr = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        ai_imc_clint_msip_before_clear[i] = 0ULL;
        ai_imc_clint_msip_after_clear[i] = 0ULL;
        ai_imc_clint_log_value[i] = 0ULL;
    }
}

static inline uint64_t ai_imc_log_count_cause(uint64_t cause)
{
    uint64_t count = 0ULL;

    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        if (ai_imc_trap_cause[i] == cause) {
            ++count;
        }
    }

    return count;
}

static inline void ai_imc_zero_pt(pte_t *pt)
{
    for (uint64_t i = 0ULL; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        pt[i] = 0ULL;
    }
}

static inline uint64_t ai_imc_satp_mode(void)
{
#ifdef sv48
    return 0x9ULL << 60;
#else
    return 0x8ULL << 60;
#endif
}

static inline uint64_t ai_imc_satp_for_root(pte_t *root)
{
    return ai_imc_satp_mode() | (((uintptr_t)root) >> PAGE_SHIFT);
}

static inline uint64_t ai_imc_satp_for_root_asid(pte_t *root,
    uint64_t asid)
{
    return ai_imc_satp_for_root(root) |
        ((asid << AI_IMC_SATP_ASID_SHIFT) & AI_IMC_SATP_ASID_MASK);
}

static inline void ai_imc_install_satp_root(pte_t *root,
    pte_t *l1,
    pte_t *l2,
    pte_t *l3,
    uintptr_t vaddr,
    uintptr_t paddr,
    uint64_t leaf_flags)
{
    ai_imc_zero_pt(root);
    ai_imc_zero_pt(l1);
    ai_imc_zero_pt(l2);
    ai_imc_zero_pt(l3);

#ifdef sv48
    uintptr_t addr = 0ULL;

    root[PTE_INDEX(0, vaddr)] = PTE_V | (((uintptr_t)l1) >> 2U);
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        l1[i] = (addr >> 2U) | leaf_flags;
        addr += SUPERPAGE_SIZE(1);
    }
    l1[PTE_INDEX(1, vaddr)] = PTE_V | (((uintptr_t)l2) >> 2U);
    l2[PTE_INDEX(2, vaddr)] = PTE_V | (((uintptr_t)l3) >> 2U);
    l3[PTE_INDEX(3, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#else
    uintptr_t addr = 0ULL;

    (void)l3;
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        root[i] = (addr >> 2U) | leaf_flags;
        addr += SUPERPAGE_SIZE(0);
    }
    root[PTE_INDEX(0, vaddr)] = PTE_V | (((uintptr_t)l1) >> 2U);
    l1[PTE_INDEX(1, vaddr)] = PTE_V | (((uintptr_t)l2) >> 2U);
    l2[PTE_INDEX(2, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#endif
}

static inline void ai_imc_add_satp_leaf(pte_t *l2,
    pte_t *l3,
    uintptr_t vaddr,
    uintptr_t paddr,
    uint64_t leaf_flags)
{
#ifdef sv48
    (void)l2;
    l3[PTE_INDEX(3, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#else
    (void)l3;
    l2[PTE_INDEX(2, vaddr)] =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#endif
}

static inline uint64_t ai_imc_pmp_window_cfg(uint64_t middle_cfg)
{
    return AI_IMC_PMP_TOR_RWX | (middle_cfg << 8U) |
        (AI_IMC_PMP_TOR_RWX << 16U);
}

static inline uint64_t ai_imc_pmp_napot4k_addr(uintptr_t page_base)
{
    return (page_base >> 2U) | ((PAGE_SIZE / 2U - 1U) >> 2U);
}

static inline uint64_t ai_imc_pmp_overlap_allow_all_cfg(uint64_t entry0_cfg)
{
    return entry0_cfg | (AI_IMC_PMP_TOR_RWX << 16U);
}

static inline void ai_imc_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_IMC_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_imc_restore_pmp_state(uint64_t saved_pmpcfg0,
    uint64_t saved_pmpaddr0,
    uint64_t saved_pmpaddr1,
    uint64_t saved_pmpaddr2)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, saved_pmpaddr0);
    CSRW(CSR_PMPADDR1, saved_pmpaddr1);
    CSRW(CSR_PMPADDR2, saved_pmpaddr2);
    CSRW(CSR_PMPCFG0, saved_pmpcfg0);
}

static inline void ai_imc_configure_pmp_window(uintptr_t start,
    uintptr_t end,
    uint64_t middle_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2U);
    CSRW(CSR_PMPADDR1, end >> 2U);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_imc_pmp_window_cfg(middle_cfg));
}

static inline void ai_imc_prepare_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
}

static inline void ai_imc_enable_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IMC_MSTATUS_MPP_MASK);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_imc_clear_mprv_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IMC_MSTATUS_MPP_MASK);
}

static inline pte_t *ai_imc_hs_leaf_entry(enum test_page page)
{
#if defined(sv39)
    return &hspt[2][page];
#else
    return &hspt[4][page];
#endif
}

static inline void ai_imc_set_hs_leaf(enum test_page page,
    pte_t flags)
{
    *ai_imc_hs_leaf_entry(page) =
        (phys_page_base(page) >> 2U) | flags;
    sfence_vma();
}

static inline void ai_imc_write_hs_leaf_to_page(enum test_page va_page,
    enum test_page pa_page,
    pte_t flags,
    uint64_t do_sfence)
{
    *ai_imc_hs_leaf_entry(va_page) =
        (phys_page_base(pa_page) >> 2U) | flags;
    fence_iorw_iorw();
    if (do_sfence) {
        sfence_vma();
    }
}

static inline void ai_imc_write_hs_leaf_to_paddr(enum test_page va_page,
    uintptr_t paddr,
    pte_t flags,
    uint64_t do_sfence)
{
    *ai_imc_hs_leaf_entry(va_page) =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | flags;
    fence_iorw_iorw();
    if (do_sfence) {
        sfence_vma();
    }
}

static inline void ai_imc_prepare_pbmt_io_mem_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, AI_IMC_PBMT_IO_MEM_BASE >> 2U);
    CSRW(CSR_PMAADDR1, AI_IMC_PBMT_IO_MEM_LIMIT >> 2U);
    CSRS(CSR_PMACFG0, 1ULL << 8);
    CSRS(CSR_PMACFG0, 1ULL << 9);
    CSRS(CSR_PMACFG0, 1ULL << 10);
    CSRS(CSR_PMACFG0, 1ULL << 11);
    CSRC(CSR_PMACFG0, 1ULL << 14);
    fence_iorw_iorw();
}

static inline void ai_imc_reset_pbmt_io_mem_window(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMAADDR0, 0xFFFFFFFFFFFULL);
    CSRW(CSR_PMACFG0, 0x6FULL);

    CSRW(CSR_PMACFG2, 0xF006F0F00000000ULL);
    CSRW(CSR_PMAADDR12, 0x20000000ULL);
    CSRW(CSR_PMAADDR13, 0x120000000ULL);
    CSRW(CSR_PMAADDR14, 0xC000000000ULL);
    CSRW(CSR_PMAADDR15, 0x10000000000ULL);

    CSRW(CSR_PMACFG0, 0ULL);
    fence_iorw_iorw();
}

static inline uint64_t ai_imc_pma_cfg_entry(unsigned entry, uint64_t bits)
{
    return bits << (entry * 8U);
}

static inline uint64_t ai_imc_pma_tor_entry1_cfg(uint64_t bits)
{
    return ai_imc_pma_cfg_entry(1U, bits);
}

static inline uintptr_t ai_imc_make_noncanonical_vaddr(uintptr_t canonical_vaddr)
{
#ifdef sv48
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit47 = (canonical_vaddr >> 47U) & 1ULL;

    if (bit47) {
        bad_vaddr &= ~(1ULL << 48U);
    } else {
        bad_vaddr |= (1ULL << 48U);
    }
#else
    uintptr_t bad_vaddr = canonical_vaddr;
    uintptr_t bit38 = (canonical_vaddr >> 38U) & 1ULL;

    if (bit38) {
        bad_vaddr &= ~(1ULL << 39U);
    } else {
        bad_vaddr |= (1ULL << 39U);
    }
#endif

    return bad_vaddr;
}

static inline uint64_t ai_imc_ld_probe(uintptr_t addr, uint64_t sentinel)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv %[value], %[sentinel]\n\t"
        "ld %[value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr),
          [sentinel] "r"(sentinel)
        : "memory");

    return value;
}

static inline void ai_imc_replay_pair_capture(uintptr_t store_vaddr,
    uintptr_t load0_vaddr, uintptr_t load1_vaddr, uint64_t payload,
    uint64_t *load0_value, uint64_t *load1_value)
{
    sd(store_vaddr, payload);
    *load0_value = ai_imc_ld_probe(load0_vaddr, 0ULL);
    *load1_value = ai_imc_ld_probe(load1_vaddr, 0ULL);
}

static inline void ai_imc_fast_replay_vector_priority_window(
    uintptr_t cold_vaddr,
    uintptr_t vec_src_vaddr,
    uintptr_t vec_dst_vaddr,
    uintptr_t pressure_vaddr,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    uint64_t *cold_value_out,
    uint64_t *pressure_mix_out)
{
    uint64_t cold_value = 0ULL;
    uint64_t pressure_mix = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[cold_value], 0(%[cold])\n\t"
        "csrs sip, %[ssip]\n\t"
        "csrs sstatus, %[sie]\n\t"
        "vsetvli t0, %[vl], e8, m1, ta, ma\n\t"
        "vle8.v v8, (%[vec_src])\n\t"
        "ld t1, 0(%[pressure])\n\t"
        "ld t2, 8(%[pressure])\n\t"
        "xor t1, t1, t2\n\t"
        "mv %[pressure_mix], t1\n\t"
        "vse8.v v8, (%[vec_dst])\n\t"
        "sd t1, 8(%[marker])\n\t"
        "sd %[marker_value], 0(%[marker])\n\t"
        ".option pop\n\t"
        : [cold_value] "=&r"(cold_value),
          [pressure_mix] "=&r"(pressure_mix)
        : [cold] "r"(cold_vaddr),
          [vec_src] "r"(vec_src_vaddr),
          [vec_dst] "r"(vec_dst_vaddr),
          [pressure] "r"(pressure_vaddr),
          [marker] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [ssip] "r"(AI_IMC_SIP_SSIP),
          [sie] "r"(SSTATUS_SIE_BIT),
          [vl] "r"((unsigned)AI_IMC_FAST_REPLAY_VEC_BYTES)
        : "t0", "t1", "t2", "memory");

    *cold_value_out = cold_value;
    *pressure_mix_out = pressure_mix;
}

static inline uint64_t ai_imc_lwu_probe(uintptr_t addr, uint64_t sentinel)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv %[value], %[sentinel]\n\t"
        "lwu %[value], 0(%[addr])\n\t"
        ".option pop\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr),
          [sentinel] "r"(sentinel)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lb_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lb %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lbu_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lbu %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lh_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lh %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lhu_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lhu %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lw_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lw %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lwu_raw64(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        "lwu %[value], 0(%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline void ai_imc_write_bytes(uintptr_t addr,
    const uint8_t *bytes,
    uint64_t len)
{
    for (uint64_t i = 0ULL; i < len; ++i) {
        write8(addr + i, bytes[i]);
    }
}

static inline bool ai_imc_bytes_match(uintptr_t addr,
    const uint8_t *bytes,
    uint64_t len)
{
    for (uint64_t i = 0ULL; i < len; ++i) {
        if (read8(addr + i) != bytes[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_imc_words_match(uintptr_t addr,
    const uint32_t *words,
    uint64_t count)
{
    for (uint64_t i = 0ULL; i < count; ++i) {
        if (read32(addr + i * sizeof(uint32_t)) != words[i]) {
            return false;
        }
    }

    return true;
}

static inline bool ai_imc_words_match4(uintptr_t addr,
    uint32_t w0,
    uint32_t w1,
    uint32_t w2,
    uint32_t w3)
{
    return read32(addr) == w0 &&
        read32(addr + 4U) == w1 &&
        read32(addr + 8U) == w2 &&
        read32(addr + 12U) == w3;
}

static inline bool ai_imc_bytes_match_array(uintptr_t addr,
    const uint8_t *bytes,
    uint64_t count,
    uint64_t *bad_index)
{
    for (uint64_t i = 0ULL; i < count; ++i) {
        if (read8(addr + i) != bytes[i]) {
            *bad_index = i;
            return false;
        }
    }

    *bad_index = count;
    return true;
}

static inline uint64_t ai_imc_read_vlenb(void)
{
    uint64_t vlenb;

    asm volatile("csrr %0, vlenb" : "=r"(vlenb));
    return vlenb;
}

static inline void ai_imc_enable_vector_state(void)
{
    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, AI_IMC_MSTATUS_VS_DIRTY);
    CSRW(CSR_VSTART, 0ULL);
}

static inline uint64_t ai_imc_lr_d(uint64_t *addr)
{
    uint64_t value;

    asm volatile(
        "lr.d %[value], (%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_lr_d_addr(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value = sentinel;

    asm volatile(
        "lr.d %[value], (%[addr])\n\t"
        : [value] "+&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_sc_d(uint64_t *addr, uint64_t value)
{
    uint64_t status;

    asm volatile(
        "sc.d %[status], %[value], (%[addr])\n\t"
        : [status] "=&r"(status)
        : [addr] "r"(addr),
          [value] "r"(value)
        : "memory");

    return status;
}

static inline uint64_t ai_imc_sc_d_addr(uintptr_t addr, uint64_t value)
{
    uint64_t status;

    asm volatile(
        "sc.d %[status], %[value], (%[addr])\n\t"
        : [status] "=&r"(status)
        : [addr] "r"(addr),
          [value] "r"(value)
        : "memory");

    return status;
}

static inline uint64_t ai_imc_amoadd_d(uint64_t *addr, uint64_t addend)
{
    uint64_t old_value;

    asm volatile(
        "amoadd.d %[old], %[addend], (%[addr])\n\t"
        : [old] "=&r"(old_value)
        : [addr] "r"(addr),
          [addend] "r"(addend)
        : "memory");

    return old_value;
}

static inline uint64_t ai_imc_amoadd_d_addr(uintptr_t addr,
    uint64_t addend,
    uint64_t sentinel)
{
    uint64_t old_value = sentinel;

    asm volatile(
        "amoadd.d %[old], %[addend], (%[addr])\n\t"
        : [old] "+&r"(old_value)
        : [addr] "r"(addr),
          [addend] "r"(addend)
        : "memory");

    return old_value;
}

static inline uint64_t ai_imc_amoadd_d_aqrl(uint64_t *addr,
    uint64_t addend)
{
    uint64_t old_value;

    asm volatile(
        "amoadd.d.aqrl %[old], %[addend], (%[addr])\n\t"
        : [old] "=&r"(old_value)
        : [addr] "r"(addr),
          [addend] "r"(addend)
        : "memory");

    return old_value;
}

static inline uint64_t ai_imc_sign_extend_w(uint32_t value)
{
    return (uint64_t)(int64_t)(int32_t)value;
}

static inline uint64_t ai_imc_amoadd_w_raw64(uint32_t *addr,
    uint32_t addend)
{
    uint64_t old_value;
    uint64_t operand = addend;

    asm volatile(
        "amoadd.w %[old], %[addend], (%[addr])\n\t"
        : [old] "=&r"(old_value)
        : [addr] "r"(addr),
          [addend] "r"(operand)
        : "memory");

    return old_value;
}

static inline uint64_t ai_imc_lr_w_raw64(uint32_t *addr)
{
    uint64_t value;

    asm volatile(
        "lr.w %[value], (%[addr])\n\t"
        : [value] "=&r"(value)
        : [addr] "r"(addr)
        : "memory");

    return value;
}

static inline uint64_t ai_imc_sc_w_raw64(uint32_t *addr, uint32_t value)
{
    uint64_t status;
    uint64_t operand = value;

    asm volatile(
        "sc.w %[status], %[value], (%[addr])\n\t"
        : [status] "=&r"(status)
        : [addr] "r"(addr),
          [value] "r"(operand)
        : "memory");

    return status;
}

static inline void ai_imc_load8_signature(uintptr_t base,
    uint64_t *sum,
    uint64_t *xor_value)
{
    uint64_t r0;
    uint64_t r1;
    uint64_t r2;
    uint64_t r3;
    uint64_t r4;
    uint64_t r5;
    uint64_t r6;
    uint64_t r7;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[r0], 0(%[base])\n\t"
        "ld %[r1], 8(%[base])\n\t"
        "ld %[r2], 16(%[base])\n\t"
        "ld %[r3], 24(%[base])\n\t"
        "ld %[r4], 32(%[base])\n\t"
        "ld %[r5], 40(%[base])\n\t"
        "ld %[r6], 48(%[base])\n\t"
        "ld %[r7], 56(%[base])\n\t"
        ".option pop\n\t"
        : [r0] "=&r"(r0),
          [r1] "=&r"(r1),
          [r2] "=&r"(r2),
          [r3] "=&r"(r3),
          [r4] "=&r"(r4),
          [r5] "=&r"(r5),
          [r6] "=&r"(r6),
          [r7] "=&r"(r7)
        : [base] "r"(base)
        : "memory");

    *sum = r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7;
    *xor_value = r0 ^ r1 ^ r2 ^ r3 ^ r4 ^ r5 ^ r6 ^ r7;
}

typedef struct {
    uint64_t loaded;
    uintptr_t cld_pc;
    uintptr_t cld_next;
    uintptr_t csw_pc;
    uintptr_t csw_next;
    uintptr_t csd_pc;
    uintptr_t csd_next;
} ai_imc_rvc_probe_result_t;

typedef struct {
    uint64_t loaded;
    uintptr_t cfld_pc;
    uintptr_t cfld_next;
    uintptr_t cfsd_pc;
    uintptr_t cfsd_next;
} ai_imc_rvc_fp_probe_result_t;

typedef struct {
    uintptr_t pc;
    uintptr_t next_pc;
} ai_imc_rvc_store_fault_pc_t;

static inline void ai_imc_rvc_memory_probe(uintptr_t target,
    uintptr_t sd_target,
    uint64_t sd_value,
    uint32_t sw_value,
    ai_imc_rvc_probe_result_t *result)
{
    asm volatile(
        ".option push\n\t"
        ".option rvc\n\t"
        "mv a0, %[target]\n\t"
        "mv a2, %[sd_target]\n\t"
        "mv a3, %[sd_value]\n\t"
        "mv a4, %[sw_value]\n\t"
        "1:\n\t"
        "c.ld a1, 0(a0)\n\t"
        "2:\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "3:\n\t"
        "c.sw a4, 0(a0)\n\t"
        "4:\n\t"
        "c.sd a3, 0(a2)\n\t"
        "5:\n\t"
        "sd a1, 0(%[result])\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 8(%[result])\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 16(%[result])\n\t"
        "lla t0, 3b\n\t"
        "sd t0, 24(%[result])\n\t"
        "lla t0, 4b\n\t"
        "sd t0, 32(%[result])\n\t"
        "lla t0, 4b\n\t"
        "sd t0, 40(%[result])\n\t"
        "lla t0, 5b\n\t"
        "sd t0, 48(%[result])\n\t"
        ".option pop\n\t"
        :
        : [target] "r"(target),
          [sd_target] "r"(sd_target),
          [sd_value] "r"(sd_value),
          [sw_value] "r"((uint64_t)sw_value),
          [result] "r"(result),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "a0", "a1", "a2", "a3", "a4", "t0", "memory");
}

static inline void ai_imc_rvc_fp_load_store_probe(uintptr_t load_base,
    uintptr_t store_base,
    ai_imc_rvc_fp_probe_result_t *result)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[load_base]\n\t"
        "mv t3, %[store_base]\n\t"
        "mv t4, %[result]\n\t"
        "mv t5, s0\n\t"
        "mv s0, t2\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.fld fs0, 16(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "mv s0, t3\n\t"
        ".option rvc\n\t"
        "3:\n\t"
        "c.fsd fs1, 24(s0)\n\t"
        "4:\n\t"
        ".option norvc\n\t"
        "fmv.x.d t1, fs0\n\t"
        "sd t1, 0(t4)\n\t"
        "lla t0, 1b\n\t"
        "sd t0, 8(t4)\n\t"
        "lla t0, 2b\n\t"
        "sd t0, 16(t4)\n\t"
        "lla t0, 3b\n\t"
        "sd t0, 24(t4)\n\t"
        "lla t0, 4b\n\t"
        "sd t0, 32(t4)\n\t"
        "mv s0, t5\n\t"
        ".option pop\n\t"
        :
        : [load_base] "r"(load_base),
          [store_base] "r"(store_base),
          [result] "r"(result),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "t0", "t1", "t2", "t3", "t4", "t5",
          "fs0", "memory");
}

static inline void ai_imc_rvc_csd_marker_probe(uintptr_t store_vaddr,
    uint64_t payload,
    uintptr_t marker_vaddr,
    uint64_t marker_value,
    ai_imc_rvc_store_fault_pc_t *pc_info)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv t2, %[store_vaddr]\n\t"
        "mv t3, %[payload]\n\t"
        "mv t4, %[marker_vaddr]\n\t"
        "mv t5, %[marker_value]\n\t"
        "mv a6, %[pc_info]\n\t"
        "mv a4, s0\n\t"
        "mv a5, s1\n\t"
        "mv s0, t2\n\t"
        "mv s1, t3\n\t"
        "lla t0, 1f\n\t"
        "sd t0, 0(a6)\n\t"
        "lla t0, 2f\n\t"
        "sd t0, 8(a6)\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        "c.sd s1, 0(s0)\n\t"
        "2:\n\t"
        ".option norvc\n\t"
        "sd t5, 0(t4)\n\t"
        "mv s0, a4\n\t"
        "mv s1, a5\n\t"
        ".option pop\n\t"
        :
        : [store_vaddr] "r"(store_vaddr),
          [payload] "r"(payload),
          [marker_vaddr] "r"(marker_vaddr),
          [marker_value] "r"(marker_value),
          [pc_info] "r"(pc_info)
        : "a4", "a5", "a6", "t0", "t2", "t3", "t4", "t5",
          "memory");
}

static inline void ai_imc_vec_active_025_load_dump(const uint32_t *old_src,
    const uint32_t *new_src,
    uint32_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vmseq.vi v2, v4, 2\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vmseq.vi v2, v4, 5\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vle32.v v6, (%[new_src]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [old_src] "r"(old_src),
          [new_src] "r"(new_src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_IMC_VEC_VL6)
        : "t0", "memory");
}

static inline void ai_imc_vec_active_025_store(const uint32_t *src,
    uint32_t *target,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vmseq.vi v2, v4, 2\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vmseq.vi v2, v4, 5\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vse32.v v6, (%[target]), v0.t\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target),
          [vl] "r"((unsigned)AI_IMC_VEC_VL6)
        : "t0", "memory");
}

static inline void ai_imc_mask_load_v0_e8_vl9(const uint8_t *mask,
    uint64_t *out_vl,
    uint64_t *out_vtype,
    uint64_t *out_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlm.v v0, (%[mask])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vtype], vtype\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(*out_vl),
          [out_vtype] "=&r"(*out_vtype),
          [out_vstart] "=&r"(*out_vstart)
        : [mask] "r"(mask),
          [vl] "r"((unsigned)AI_IMC_MASK_VL9)
        : "t0", "memory");
}

static inline void ai_imc_mask_dump_and_store_e8_vl9(const uint8_t *src,
    uint8_t *raw_dump,
    uint8_t *dst,
    uint64_t *out_vl,
    uint64_t *out_vtype,
    uint64_t *out_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vsm.v v0, (%[raw_dump])\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vse8.v v8, (%[dst]), v0.t\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vtype], vtype\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(*out_vl),
          [out_vtype] "=&r"(*out_vtype),
          [out_vstart] "=&r"(*out_vstart)
        : [src] "r"(src),
          [raw_dump] "r"(raw_dump),
          [dst] "r"(dst),
          [vl] "r"((unsigned)AI_IMC_MASK_VL9)
        : "t0", "memory");
}

static inline void ai_imc_mask_store_expected_u8(uint8_t *expected,
    const uint8_t *seed,
    const uint8_t *src,
    const uint8_t *mask,
    uint64_t count)
{
    for (uint64_t i = 0ULL; i < count; ++i) {
        expected[i] = seed[i];
        if (i < AI_IMC_MASK_VL9 &&
            ((mask[i >> 3U] >> (i & 7U)) & 1U) != 0U) {
            expected[i] = src[i];
        }
    }
}

static inline void ai_imc_mask_store_expected_u16(uint16_t *expected,
    const uint16_t *seed,
    const uint16_t *src,
    const uint8_t *mask,
    uint64_t count)
{
    for (uint64_t i = 0ULL; i < count; ++i) {
        expected[i] = seed[i];
        if (i < AI_IMC_MASK_VL9 &&
            ((mask[i >> 3U] >> (i & 7U)) & 1U) != 0U) {
            expected[i] = src[i];
        }
    }
}

static inline bool ai_imc_halfwords_match(uintptr_t addr,
    const uint16_t *words,
    uint64_t count)
{
    for (uint64_t i = 0ULL; i < count; ++i) {
        if (read16(addr + i * sizeof(uint16_t)) != words[i]) {
            return false;
        }
    }

    return true;
}

static inline void ai_imc_vec_store_mask_024(const uint32_t *src,
    uint32_t *target,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vmseq.vi v2, v4, 2\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vmseq.vi v2, v4, 4\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vse32.v v6, (%[target]), v0.t\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target),
          [vl] "r"((unsigned)AI_IMC_VEC_VL6)
        : "t0", "memory");
}

static inline void ai_imc_vec_store_mask_134(const uint32_t *src,
    uint32_t *target,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 1\n\t"
        "vmseq.vi v2, v4, 3\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vmseq.vi v2, v4, 4\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vse32.v v6, (%[target]), v0.t\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target),
          [vl] "r"((unsigned)AI_IMC_VEC_VL6)
        : "t0", "memory");
}

static inline void ai_imc_vec_prepare_e32m1_vl4(void)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        ".option pop\n\t"
        :
        : [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_copy_current_vl_u32(const uint32_t *src,
    uint32_t *target,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target)
        : "memory");
}

static inline void ai_imc_vec_load_current_vl_u32(const uint32_t *src,
    uint64_t *out_vl,
    uint64_t *out_vtype,
    uint64_t *out_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vle32.v v6, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vtype], vtype\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(*out_vl),
          [out_vtype] "=&r"(*out_vtype),
          [out_vstart] "=&r"(*out_vstart)
        : [src] "r"(src)
        : "memory");
}

static inline void ai_imc_vec_store_current_vl_u32(uint32_t *target,
    uint64_t *out_vl,
    uint64_t *out_vtype,
    uint64_t *out_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vtype], vtype\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(*out_vl),
          [out_vtype] "=&r"(*out_vtype),
          [out_vstart] "=&r"(*out_vstart)
        : [target] "r"(target)
        : "memory");
}

static inline void ai_imc_vec_vse32_v6_vl4_from_vstart(uint32_t *target,
    uint64_t start,
    uint64_t *final_vl,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(*final_vl),
          [out_vstart] "=&r"(*final_vstart)
        : [target] "r"(target),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline uintptr_t ai_imc_hs_page_offset(enum test_page base_page,
    enum test_page target_page,
    uintptr_t offset)
{
    return hs_page_base(target_page) + offset - hs_page_base(base_page);
}

static inline void ai_imc_vec_ordered_indexed_load64_seeded(
    const uint64_t *sentinel,
    const uint64_t *offsets,
    const uint8_t *base,
    uint64_t start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v6, (%[sentinel])\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [sentinel] "r"(sentinel),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_ordered_indexed_load64_resume(
    const uint64_t *offsets,
    const uint8_t *base,
    uint64_t start)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "csrw vstart, %[start]\n\t"
        "vloxei64.v v6, (%[base]), v8\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [offsets] "r"(offsets),
          [base] "r"(base),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_ordered_indexed_dump_v6(uint64_t *dump)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vse64.v v6, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        :
        : [dump] "r"(dump),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_masked_indexed_even_load(const uint8_t *base,
    const uint64_t *offsets,
    const uint32_t *old_src,
    uint32_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vluxei64.v v6, (%[base]), v8, v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [old_src] "r"(old_src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_masked_indexed_even_store(const uint32_t *src,
    const uint64_t *offsets,
    uint8_t *base,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vand.vi v4, v4, 1\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vsuxei64.v v6, (%[base]), v8, v0.t\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [offsets] "r"(offsets),
          [base] "r"(base),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_masked_vle32_lane0_dump(
    const uint32_t *old_src,
    const uint32_t *src,
    uint32_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vle32.v v6, (%[src]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [old_src] "r"(old_src),
          [src] "r"(src),
          [dump] "r"(dump),
          [vl] "r"(2U)
        : "t0", "memory");
}

static inline void ai_imc_vec_tail_load_dump(const uint32_t *old_src,
    const uint32_t *new_src,
    uint32_t *dump,
    uint64_t short_vl,
    uint64_t agnostic,
    uint64_t *final_vstart)
{
    if (agnostic) {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vsetvli t0, %[short_vl], e32, m1, ta, mu\n\t"
            "vle32.v v6, (%[new_src])\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "csrr %[final_vstart], vstart\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [final_vstart] "=&r"(*final_vstart)
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [full_vl] "r"((unsigned)AI_IMC_VEC_VL4),
              [short_vl] "r"(short_vl)
            : "t0", "memory");
    } else {
        asm volatile(
            ".option push\n\t"
            ".option norvc\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "csrw vstart, zero\n\t"
            "vle32.v v6, (%[old_src])\n\t"
            "vsetvli t0, %[short_vl], e32, m1, tu, mu\n\t"
            "vle32.v v6, (%[new_src])\n\t"
            "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
            "vse32.v v6, (%[dump])\n\t"
            "csrr %[final_vstart], vstart\n\t"
            "fence rw, rw\n\t"
            ".option pop\n\t"
            : [final_vstart] "=&r"(*final_vstart)
            : [old_src] "r"(old_src),
              [new_src] "r"(new_src),
              [dump] "r"(dump),
              [full_vl] "r"((unsigned)AI_IMC_VEC_VL4),
              [short_vl] "r"(short_vl)
            : "t0", "memory");
    }
}

static inline void ai_imc_vec_masked_tail_load_dump(const uint32_t *old_src,
    const uint32_t *new_src,
    uint32_t *dump,
    uint64_t short_vl,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vsetvli t0, %[short_vl], e32, m1, tu, mu\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vmseq.vi v2, v4, 2\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vle32.v v6, (%[new_src]), v0.t\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [old_src] "r"(old_src),
          [new_src] "r"(new_src),
          [dump] "r"(dump),
          [full_vl] "r"((unsigned)AI_IMC_VEC_VL4),
          [short_vl] "r"(short_vl)
        : "t0", "memory");
}

static inline void ai_imc_vec_short_store(const uint32_t *src,
    uint32_t *target,
    uint64_t short_vl,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[full_vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vsetvli t0, %[short_vl], e32, m1, tu, mu\n\t"
        "vse32.v v6, (%[target])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [target] "r"(target),
          [full_vl] "r"((unsigned)AI_IMC_VEC_VL4),
          [short_vl] "r"(short_vl)
        : "t0", "memory");
}

static inline void ai_imc_vec_ordered_indexed_duplicate_store(
    const uint8_t *base,
    const uint64_t *offsets,
    const uint32_t *src,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vid.v v4\n\t"
        "vmseq.vi v0, v4, 0\n\t"
        "vmseq.vi v2, v4, 2\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "vmseq.vi v2, v4, 3\n\t"
        "vmor.mm v0, v0, v2\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "vsoxei64.v v6, (%[base]), v8, v0.t\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [src] "r"(src),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "t0", "memory");
}

static inline void ai_imc_vec_indexed_unordered_roundtrip(const uint8_t *base,
    const uint64_t *offsets,
    const uint32_t *src,
    uint32_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m2, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle64.v v8, (%[offsets])\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "vle32.v v6, (%[src])\n\t"
        "vsuxei64.v v6, (%[base]), v8\n\t"
        "vluxei64.v v10, (%[base]), v8\n\t"
        "vse32.v v10, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [base] "r"(base),
          [offsets] "r"(offsets),
          [src] "r"(src),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_negative_stride_roundtrip(const uint16_t *load_base,
    const uint16_t *store_src,
    uint16_t *store_base,
    uint16_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e16, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlse16.v v6, (%[load_base]), %[stride]\n\t"
        "vse16.v v6, (%[dump])\n\t"
        "vle16.v v8, (%[store_src])\n\t"
        "vsse16.v v8, (%[store_base]), %[stride]\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [load_base] "r"(load_base),
          [store_src] "r"(store_src),
          [store_base] "r"(store_base),
          [dump] "r"(dump),
          [stride] "r"(-(int64_t)sizeof(uint16_t)),
          [vl] "r"((unsigned)AI_IMC_VEC_VL8)
        : "t0", "memory");
}

static inline void ai_imc_vec_seg2e8_roundtrip(const uint8_t *src,
    uint8_t *dst,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8.v v8, (%[src])\n\t"
        "vsseg2e8.v v8, (%[dst])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"((unsigned)(AI_IMC_VEC_SEG2_BYTES / 2U))
        : "t0", "memory");
}

static inline void ai_imc_vec_e8_cross64_roundtrip(const uint8_t *src,
    uint8_t *dst,
    uint8_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8.v v8, (%[src])\n\t"
        "vse8.v v8, (%[dst])\n\t"
        "vle8.v v10, (%[dst])\n\t"
        "vse8.v v10, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [dst] "r"(dst),
          [dump] "r"(dump),
          [vl] "r"(16U)
        : "t0", "memory");
}

static inline void ai_imc_vec_seg2e32_store(const uint32_t *field0,
    const uint32_t *field1,
    uint32_t *dst,
    uint64_t start,
    uint64_t *final_vstart,
    uint64_t *final_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v8, (%[field0])\n\t"
        "vle32.v v9, (%[field1])\n\t"
        "csrw vstart, %[start]\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrr %[out_vl], vl\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(*final_vstart),
          [out_vl] "=&r"(*final_vl)
        : [field0] "r"(field0),
          [field1] "r"(field1),
          [dst] "r"(dst),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_seg2e32_load_seeded(const uint32_t *sentinel0,
    const uint32_t *sentinel1,
    const uint32_t *src,
    uint64_t start,
    uint64_t *final_vstart,
    uint64_t *final_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v8, (%[sentinel0])\n\t"
        "vle32.v v9, (%[sentinel1])\n\t"
        "csrw vstart, %[start]\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrr %[out_vl], vl\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(*final_vstart),
          [out_vl] "=&r"(*final_vl)
        : [sentinel0] "r"(sentinel0),
          [sentinel1] "r"(sentinel1),
          [src] "r"(src),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_seg2e32_load_resume(const uint32_t *src,
    uint64_t start,
    uint64_t *final_vstart,
    uint64_t *final_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, %[start]\n\t"
        "vlseg2e32.v v8, (%[src])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrr %[out_vl], vl\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(*final_vstart),
          [out_vl] "=&r"(*final_vl)
        : [src] "r"(src),
          [start] "r"(start),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_seg2e32_dump_v8_v9(uint32_t *dst,
    uint64_t *final_vstart,
    uint64_t *final_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vsseg2e32.v v8, (%[dst])\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrr %[out_vl], vl\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vstart] "=&r"(*final_vstart),
          [out_vl] "=&r"(*final_vl)
        : [dst] "r"(dst),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_whole_2r_copy(const uint8_t *src,
    uint8_t *dst,
    uint64_t *vl_before_store,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e64, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vl2re8.v v6, (%[src])\n\t"
        "csrr %[vl_before_store], vl\n\t"
        "vs2r.v v6, (%[dst])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [vl_before_store] "=&r"(*vl_before_store),
          [final_vstart] "=&r"(*final_vstart)
        : [src] "r"(src),
          [dst] "r"(dst),
          [vl] "r"(2U)
        : "t0", "memory");
}

static inline void ai_imc_vec_zero_active_roundtrip(const uint32_t *old_src,
    const uint32_t *new_src,
    uint32_t *target,
    uint32_t *dump,
    uint64_t *final_vstart)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e32, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle32.v v6, (%[old_src])\n\t"
        "vle32.v v8, (%[new_src])\n\t"
        "vmxor.mm v0, v0, v0\n\t"
        "vle32.v v6, (%[new_src]), v0.t\n\t"
        "vse32.v v8, (%[target]), v0.t\n\t"
        "vse32.v v6, (%[dump])\n\t"
        "csrr %[final_vstart], vstart\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [final_vstart] "=&r"(*final_vstart)
        : [old_src] "r"(old_src),
          [new_src] "r"(new_src),
          [target] "r"(target),
          [dump] "r"(dump),
          [vl] "r"((unsigned)AI_IMC_VEC_VL4)
        : "t0", "memory");
}

static inline void ai_imc_vec_vle8ff_capture(const uint8_t *seed,
    const uint8_t *src,
    uint8_t *dump,
    uint64_t req_vl,
    uint64_t *observed_vl,
    uint64_t *observed_vstart)
{
    uint64_t captured_vl = 0ULL;
    uint64_t captured_vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vle8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vse8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed),
          [src] "r"(src),
          [dump] "r"(dump),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_imc_vec_vlseg2e8ff_capture(const uint8_t *seed,
    const uint8_t *src,
    uint8_t *dump,
    uint64_t req_vl,
    uint64_t *observed_vl,
    uint64_t *observed_vstart)
{
    uint64_t captured_vl = 0ULL;
    uint64_t captured_vstart = 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vlseg2e8.v v8, (%[seed])\n\t"
        "csrw vstart, zero\n\t"
        "vlseg2e8ff.v v8, (%[src])\n\t"
        "csrr %[out_vl], vl\n\t"
        "csrr %[out_vstart], vstart\n\t"
        "csrw vstart, zero\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "vsseg2e8.v v8, (%[dump])\n\t"
        "fence rw, rw\n\t"
        ".option pop\n\t"
        : [out_vl] "=&r"(captured_vl),
          [out_vstart] "=&r"(captured_vstart)
        : [seed] "r"(seed),
          [src] "r"(src),
          [dump] "r"(dump),
          [vl] "r"(req_vl)
        : "t0", "memory");

    *observed_vl = captured_vl;
    *observed_vstart = captured_vstart;
}

static inline void ai_imc_vec_vle8ff_element0_fault(const uint8_t *src,
    uint64_t req_vl)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "vsetvli t0, %[vl], e8, m1, tu, mu\n\t"
        "csrw vstart, zero\n\t"
        "vle8ff.v v8, (%[src])\n\t"
        ".option pop\n\t"
        :
        : [src] "r"(src),
          [vl] "r"(req_vl)
        : "t0", "memory");
}

static inline uint64_t ai_imc_fld_raw_bits(uintptr_t addr)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr)
        : "ft0", "memory");

    return value;
}

static inline void ai_imc_fld_only(uintptr_t addr)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fld ft0, 0(%[addr])\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(addr)
        : "ft0", "memory");
}

static inline uint64_t ai_imc_fld_sentinel_probe(uintptr_t addr,
    uint64_t sentinel)
{
    uint64_t value;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %[sentinel]\n\t"
        "fld ft0, 0(%[addr])\n\t"
        "fmv.x.d %[value], ft0\n\t"
        ".option pop\n\t"
        : [value] "=r"(value)
        : [addr] "r"(addr),
          [sentinel] "r"(sentinel)
        : "ft0", "memory");

    return value;
}

static inline void ai_imc_fsd_raw_bits(uintptr_t addr, uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x ft0, %[bits]\n\t"
        "fsd ft0, 0(%[addr])\n\t"
        ".option pop\n\t"
        :
        : [addr] "r"(addr),
          [bits] "r"(bits)
        : "ft0", "memory");
}

static inline void ai_imc_seed_fs1_raw_bits(uint64_t bits)
{
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "fmv.d.x fs1, %[bits]\n\t"
        ".option pop\n\t"
        :
        : [bits] "r"(bits)
        : "fs1");
}

bool ai_interrupt_memory_m_stip_disabled_load_store_then_enable_corner()
{
    TEST_START();

    const uintptr_t target_a = phys_page_base(VSRWX_GRWX) + 0x480U;
    const uintptr_t target_b = phys_page_base(VSRWX_GRWX) + 0x488U;
    const uintptr_t marker = phys_page_base(VSRWX_GRWX) + 0x490U;
    const uintptr_t guard = phys_page_base(VSRWX_GRWX) + 0x498U;
    const uint64_t seed_a = 0x696d6364736c6401ULL;
    const uint64_t seed_b = 0x696d6364736c6402ULL;
    const uint64_t value_a = 0x696d6364736c6411ULL;
    const uint64_t value_b = 0x696d6364736c6422ULL;
    const uint64_t marker_value = 0x696d6364736c6433ULL;
    const uint64_t guard_seed = 0x696d6364736c64aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t load_before_enable = 0ULL;
    uint64_t mip_before_enable = 0ULL;
    uint64_t mip_after_trap = 0ULL;
    uint64_t rounds = 0ULL;
    bool disabled_window_no_trap = false;

    write64(target_a, seed_a);
    write64(target_b, seed_b);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    write64(target_a, value_a);
    load_before_enable = read64(target_a);
    write64(target_b, value_b);
    write64(marker, marker_value);
    fence_iorw_iorw();
    mip_before_enable = CSRR(CSR_MIP);
    disabled_window_no_trap = !excpt.triggered;

    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);

    const bool interrupt_seen =
        rounds < AI_IMC_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("STIP pending remains disabled while the load/store window commits",
        disabled_window_no_trap &&
        read64(target_a) == value_a &&
        read64(target_b) == value_b);
    TEST_ASSERT("the disabled window does not enter the handler before the explicit enable point",
        load_before_enable == value_a &&
        (mip_before_enable & AI_IMC_MIP_STIP) != 0ULL &&
        read64(marker) == marker_value);
    TEST_ASSERT("after enable, the preserved STIP is delivered once and the handler clears the source",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("disabled-window memory side effects and guard survive interrupt delivery",
        read64(target_a) == value_a &&
        read64(target_b) == value_b &&
        read64(guard) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_disabled_load_store_then_enable_corner);
}

bool ai_interrupt_memory_m_stip_pre_first_load_store_precise_return_corner()
{
    TEST_START();

    const uintptr_t load_addr = phys_page_base(SWITCH1) + 0x4a0U;
    const uintptr_t store_addr = phys_page_base(SWITCH1) + 0x4a8U;
    const uintptr_t marker_addr = phys_page_base(SWITCH1) + 0x4b0U;
    const uintptr_t guard_addr = phys_page_base(SWITCH1) + 0x4b8U;
    const uint64_t load_seed = 0x696d637072657401ULL;
    const uint64_t store_seed = 0x696d637072657402ULL;
    const uint64_t store_value = 0x696d637072657411ULL;
    const uint64_t marker_value = 0x696d637072657422ULL;
    const uint64_t guard_seed = 0x696d6370726574aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t loaded = 0ULL;
    uint64_t epc_snapshot = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(load_addr, load_seed);
    write64(store_addr, store_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    epc_snapshot = excpt.epc;

    loaded = read64(load_addr);
    write64(store_addr, store_value);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("enabled STIP is observed before the first target load/store sequence and clears its pending source",
        interrupt_seen &&
        epc_snapshot != 0ULL &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("after interrupt return, the first load observes the seed and is not skipped",
        loaded == load_seed);
    TEST_ASSERT("after interrupt return, the first store and marker execute exactly once",
        read64(store_addr) == store_value &&
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_pre_first_load_store_precise_return_corner);
}

bool ai_interrupt_memory_m_stip_store_fence_order_marker_corner()
{
    TEST_START();

    const uintptr_t target_a = phys_page_base(SWITCH2) + 0x4c0U;
    const uintptr_t target_b = phys_page_base(SWITCH2) + 0x4c8U;
    const uintptr_t handler_log = phys_page_base(SWITCH2) + 0x4d0U;
    const uintptr_t marker = phys_page_base(SWITCH2) + 0x4d8U;
    const uint64_t seed_a = 0x696d63666e636501ULL;
    const uint64_t seed_b = 0x696d63666e636502ULL;
    const uint64_t value_a = 0x696d63666e636511ULL;
    const uint64_t value_b = 0x696d63666e636522ULL;
    const uint64_t marker_value = 0x696d63666e636533ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t read_a_after_fence = 0ULL;
    uint64_t read_b_after_fence = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(target_a, seed_a);
    write64(target_b, seed_b);
    write64(handler_log, 0ULL);
    write64(marker, 0ULL);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    TEST_SETUP_EXCEPT();
    write64(target_a, value_a);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);

    write64(target_b, value_b);
    fence_iorw_iorw();
    read_a_after_fence = read64(target_a);
    read_b_after_fence = read64(target_b);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("STIP is delivered in the store/fence window and then cleared",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("fence after interrupt return orders both mainline stores before readback",
        read_a_after_fence == value_a &&
        read_b_after_fence == value_b &&
        read64(target_a) == value_a &&
        read64(target_b) == value_b);
    TEST_ASSERT("the handler bookkeeping path does not replace the mainline fence marker",
        read64(handler_log) == 0ULL &&
        read64(marker) == marker_value);

    TEST_END(ai_interrupt_memory_m_stip_store_fence_order_marker_corner);
}

bool ai_interrupt_memory_m_stip_fence_handler_log_marker_owner_corner()
{
    TEST_START();

    const uintptr_t target_a = phys_page_base(SWITCH2) + 0x1e80U;
    const uintptr_t target_b = phys_page_base(SWITCH2) + 0x1e88U;
    const uintptr_t main_marker = phys_page_base(SWITCH2) + 0x1e90U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x1e98U;
    const uint64_t seed_a = 0x696d63666c6f6101ULL;
    const uint64_t seed_b = 0x696d63666c6f6202ULL;
    const uint64_t value_a = 0x696d63666c6f6111ULL;
    const uint64_t value_b = 0x696d63666c6f6222ULL;
    const uint64_t marker_value = 0x696d63666c6f6d33ULL;
    const uint64_t guard_seed = 0x696d63666c6faa04ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t marker_after_handlers = 0ULL;
    uint64_t b_before_main_write = 0ULL;
    uint64_t read_a_after_fence = 0ULL;

    goto_priv(PRIV_M);
    write64(target_a, seed_a);
    write64(target_b, seed_b);
    write64(main_marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(target_a, value_a);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    marker_after_handlers = read64(main_marker);
    b_before_main_write = read64(target_b);
    read_a_after_fence = read64(target_a);
    write64(target_b, value_b);
    write64(main_marker, marker_value);
    fence_iorw_iorw();

    const bool interrupt_log_exact =
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool handler_log_not_main_marker =
        marker_after_handlers == 0ULL &&
        b_before_main_write == seed_b &&
        read64(main_marker) == marker_value &&
        read64(main_marker) != ai_imc_handler_log_page[0] &&
        read64(main_marker) != ai_imc_handler_log_page[1];
    const bool mainline_fence_order_ok =
        read_a_after_fence == value_a &&
        read64(target_a) == value_a &&
        read64(target_b) == value_b &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("two STIP injections around the mainline fence each create one independent handler log store and clear source state",
        interrupt_log_exact,
        "trap_count=%lx cause0=%lx cause1=%lx log0=%lx log1=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1], CSRR(CSR_MIP));
    TEST_ASSERT("handler log stores do not substitute for the mainline ordering marker or post-fence store",
        handler_log_not_main_marker,
        "marker_after_handlers=%lx marker=%lx b_before=%lx b_final=%lx log0=%lx log1=%lx",
        marker_after_handlers, read64(main_marker), b_before_main_write,
        read64(target_b), ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1]);
    TEST_ASSERT("mainline fence orders the pre-fence store before the post-fence load/store marker episode",
        mainline_fence_order_ok,
        "read_a=%lx a=%lx b=%lx guard=%lx",
        read_a_after_fence, read64(target_a), read64(target_b),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_fence_handler_log_marker_owner_corner);
}

bool ai_interrupt_memory_m_stip_amo_lrsc_contract_corner()
{
    TEST_START();

    const uintptr_t amo_addr = phys_page_base(SCRATCHPAD) + 0x500U;
    const uintptr_t lrsc_addr = phys_page_base(SCRATCHPAD) + 0x508U;
    const uintptr_t marker_addr = phys_page_base(SCRATCHPAD) + 0x510U;
    const uintptr_t guard_addr = phys_page_base(SCRATCHPAD) + 0x518U;
    const uint64_t amo_seed = 0x696d63616d6f0001ULL;
    const uint64_t amo_addend = 0x123ULL;
    const uint64_t lrsc_seed = 0x696d636c72736301ULL;
    const uint64_t lrsc_value = 0x696d636c72736311ULL;
    const uint64_t marker_value = 0x696d63616d6f0022ULL;
    const uint64_t guard_seed = 0x696d63616d6f00aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t amo_old = 0ULL;
    uint64_t lr_value = 0ULL;
    uint64_t sc_status = 0ULL;
    uint64_t lrsc_final = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(amo_addr, amo_seed);
    write64(lrsc_addr, lrsc_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    TEST_SETUP_EXCEPT();
    amo_old = ai_imc_amoadd_d((uint64_t *)amo_addr, amo_addend);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);

    lr_value = ai_imc_lr_d((uint64_t *)lrsc_addr);
    sc_status = ai_imc_sc_d((uint64_t *)lrsc_addr, lrsc_value);
    lrsc_final = read64(lrsc_addr);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);
    const bool sc_contract_valid =
        (sc_status == 0ULL && lrsc_final == lrsc_value) ||
        (sc_status != 0ULL && lrsc_final == lrsc_seed);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("AMO before the interrupt reports its old value and updates memory exactly once",
        amo_old == amo_seed &&
        read64(amo_addr) == amo_seed + amo_addend);
    TEST_ASSERT("STIP delivered between the AMO and LR/SC episodes is observed and cleared",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("LR/SC after interrupt return follows the reservation contract without stale success",
        lr_value == lrsc_seed &&
        sc_contract_valid);
    TEST_ASSERT("atomic episode marker and guard are not polluted by interrupt handling",
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_amo_lrsc_contract_corner);
}

bool ai_interrupt_memory_m_stip_lrsc_interrupt_window_contract_corner()
{
    TEST_START();

    const uintptr_t lrsc_addr = phys_page_base(SCRATCHPAD) + 0x580U;
    const uintptr_t marker_addr = phys_page_base(SCRATCHPAD) + 0x588U;
    const uintptr_t guard_addr = phys_page_base(SCRATCHPAD) + 0x590U;
    const uint64_t lrsc_seed = 0x696d636c72776901ULL;
    const uint64_t sc_value = 0x696d636c72776911ULL;
    const uint64_t marker_value = 0x696d636c72776922ULL;
    const uint64_t guard_seed = 0x696d636c727769aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t lr_value = 0ULL;
    uint64_t sc_status = 0ULL;
    uint64_t lrsc_final = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(lrsc_addr, lrsc_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    TEST_SETUP_EXCEPT();

    lr_value = ai_imc_lr_d((uint64_t *)lrsc_addr);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    sc_status = ai_imc_sc_d((uint64_t *)lrsc_addr, sc_value);
    lrsc_final = read64(lrsc_addr);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);
    const bool sc_contract_valid =
        (sc_status == 0ULL && lrsc_final == sc_value) ||
        (sc_status != 0ULL && lrsc_final == lrsc_seed);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("STIP is delivered between LR.D and SC.D and cleared before the post-window marker",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("the LR value is the original target seed and is not replaced by trap handler state",
        lr_value == lrsc_seed);
    TEST_ASSERT("SC after an interrupt in the reservation window either succeeds with one store or fails with no store",
        sc_contract_valid);
    TEST_ASSERT("LR/SC window marker and guard remain independent of interrupt handling",
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_lrsc_interrupt_window_contract_corner);
}

bool ai_interrupt_memory_m_stip_load_access_fault_pending_then_enable_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x600U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x600U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d636c61666601ULL;
    const uint64_t guard_seed = 0x696d636c6166aa02ULL;
    const uint64_t rd_sentinel = 0x696d636c616672ddULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_result = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_priv = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t target_after_fault = 0ULL;
    uint64_t guard_after_fault = 0ULL;
    uint64_t retry_result = 0ULL;
    uint64_t mip_after_trap = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_R);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    fault_result = ai_imc_ld_probe(vaddr, rd_sentinel);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_priv = excpt.priv;
    pending_after_fault = CSRR(CSR_MIP);
    target_after_fault = read64(paddr);
    guard_after_fault = read64(guard);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    retry_result = ai_imc_ld_probe(vaddr, rd_sentinel);
    const bool retry_load_recovers =
        excpt.triggered == false &&
        retry_result == seed &&
        read64(guard) == guard_seed;
    ai_imc_clear_mprv_translation();

    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    const bool interrupt_seen =
        rounds < AI_IMC_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("PMP denied MPRV-U translated load reports load access fault before the disabled pending STIP is delivered",
        fault_priv == PRIV_M &&
        fault_cause == CAUSE_LAF &&
        fault_tval == vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL);
    TEST_ASSERT("the faulting load keeps its destination sentinel and leaves target/guard data unchanged",
        fault_result == rd_sentinel &&
        target_after_fault == seed &&
        guard_after_fault == guard_seed);
    TEST_ASSERT("after PMP repair, the same translated load recovers and reads the original seed",
        retry_load_recovers);
    TEST_ASSERT("after the load fault is handled and repaired, the preserved STIP is delivered once and cleared",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL &&
        read64(paddr) == seed &&
        read64(guard) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_load_access_fault_pending_then_enable_corner);
}

bool ai_interrupt_memory_m_stip_store_access_fault_no_side_effect_then_enable_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x640U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x640U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d637361666601ULL;
    const uint64_t denied_payload = 0x696d6373616666ddULL;
    const uint64_t commit_payload = 0x696d637361666611ULL;
    const uint64_t guard_seed = 0x696d63736166aa02ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_priv = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t target_after_fault = 0ULL;
    uint64_t guard_after_fault = 0ULL;
    uint64_t mip_after_trap = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_priv = excpt.priv;
    pending_after_fault = CSRR(CSR_MIP);
    target_after_fault = read64(paddr);
    guard_after_fault = read64(guard);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, commit_payload);
    const bool repair_store_commits_once =
        excpt.triggered == false &&
        read64(paddr) == commit_payload &&
        read64(guard) == guard_seed;
    ai_imc_clear_mprv_translation();

    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    const bool interrupt_seen =
        rounds < AI_IMC_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("PMP denied MPRV-U translated store reports store access fault while STIP remains pending but disabled",
        fault_priv == PRIV_M &&
        fault_cause == CAUSE_SAF &&
        fault_tval == vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL);
    TEST_ASSERT("the faulting store leaks no partial payload into target or guard memory",
        target_after_fault == seed &&
        guard_after_fault == guard_seed);
    TEST_ASSERT("after PMP repair, the control store commits exactly once before pending STIP delivery",
        repair_store_commits_once);
    TEST_ASSERT("after the store fault and repair, the preserved STIP is delivered once and does not change the committed memory image",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL &&
        read64(paddr) == commit_payload &&
        read64(guard) == guard_seed);

    TEST_END(ai_interrupt_memory_m_stip_store_access_fault_no_side_effect_then_enable_corner);
}

bool ai_interrupt_memory_m_stip_mprv_u_permission_restore_corner()
{
    TEST_START();

    const enum test_page allow_page = SWITCH1;
    const enum test_page deny_page = SWITCH2;
    const uintptr_t allow_vaddr = hs_page_base(allow_page) + 0x720U;
    const uintptr_t allow_store_vaddr = hs_page_base(allow_page) + 0x728U;
    const uintptr_t deny_vaddr = hs_page_base(deny_page) + 0x730U;
    const uintptr_t allow_paddr = phys_page_base(allow_page) + 0x720U;
    const uintptr_t allow_store_paddr = phys_page_base(allow_page) + 0x728U;
    const uintptr_t deny_paddr = phys_page_base(deny_page) + 0x730U;
    const uintptr_t handler_probe = phys_page_base(SCRATCHPAD) + 0x760U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x768U;
    const uint64_t allow_seed = 0x696d636d70727601ULL;
    const uint64_t pre_store_payload = 0x696d636d70727602ULL;
    const uint64_t post_store_payload = 0x696d636d70727603ULL;
    const uint64_t deny_seed = 0x696d636d70727604ULL;
    const uint64_t handler_seed = 0x696d636d70727605ULL;
    const uint64_t guard_seed = 0x696d636d707276aaULL;
    const uint64_t deny_sentinel = 0x696d636d707276ddULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    uint64_t pre_load = 0ULL;
    uint64_t pre_store_read = 0ULL;
    uint64_t post_load = 0ULL;
    uint64_t post_store_read = 0ULL;
    uint64_t deny_load = 0ULL;
    uint64_t post_interrupt_mstatus = 0ULL;
    uint64_t deny_fault_cause = 0ULL;
    uint64_t deny_fault_tval = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(allow_paddr, allow_seed);
    write64(allow_store_paddr, 0ULL);
    write64(deny_paddr, deny_seed);
    write64(handler_probe, handler_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_mprv_user_translation();
    ai_imc_set_hs_leaf(deny_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_enable_mprv_user_translation();

    TEST_SETUP_EXCEPT();
    pre_load = ai_imc_ld_probe(allow_vaddr, 0ULL);
    sd(allow_store_vaddr, pre_store_payload);
    pre_store_read = ai_imc_ld_probe(allow_store_vaddr, 0ULL);
    const bool pre_mprv_access_ok =
        !excpt.triggered &&
        pre_load == allow_seed &&
        pre_store_read == pre_store_payload &&
        (CSRR(CSR_MSTATUS) & MSTATUS_MPRV) != 0ULL &&
        (CSRR(CSR_MSTATUS) & AI_IMC_MSTATUS_MPP_MASK) == 0ULL;

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_mprv_save_handler);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    post_interrupt_mstatus = CSRR(CSR_MSTATUS);
    CSRW(CSR_MTVEC, saved_mtvec);

    TEST_SETUP_EXCEPT();
    post_load = ai_imc_ld_probe(allow_vaddr, 0ULL);
    sd(allow_store_vaddr, post_store_payload);
    post_store_read = ai_imc_ld_probe(allow_store_vaddr, 0ULL);
    const bool post_mprv_access_ok =
        !excpt.triggered &&
        post_load == allow_seed &&
        post_store_read == post_store_payload &&
        (post_interrupt_mstatus & MSTATUS_MPRV) != 0ULL &&
        (post_interrupt_mstatus & AI_IMC_MSTATUS_MPP_MASK) == 0ULL;

    TEST_SETUP_EXCEPT();
    deny_load = ai_imc_ld_probe(deny_vaddr, deny_sentinel);
    deny_fault_cause = excpt.cause;
    deny_fault_tval = excpt.tval;
    ai_imc_clear_mprv_translation();
    const bool deny_page_fault_uses_main_mprv =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        deny_fault_cause == CAUSE_LPF &&
        deny_fault_tval == deny_vaddr &&
        deny_load == deny_sentinel &&
        read64(deny_paddr) == deny_seed &&
        read64(guard) == guard_seed;

    const bool interrupt_handler_kept_context =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_handler_mstatus[0] & MSTATUS_MPRV) != 0ULL &&
        (ai_imc_handler_mstatus[0] & AI_IMC_MSTATUS_MPP_MASK) ==
            AI_IMC_MSTATUS_MPP_M &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("baseline MPRV-U load/store succeeds on the user-accessible page before the interrupt",
        pre_mprv_access_ok,
        "pre_load=%lx seed=%lx store_read=%lx mstatus=%lx",
        pre_load, allow_seed, pre_store_read,
        CSRR(CSR_MSTATUS));
    TEST_ASSERT("STIP taken while MPRV is active records the incoming MPRV context and keeps handler memory separate",
        interrupt_handler_kept_context,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx saved_status=%lx probe=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_mstatus[0],
        ai_imc_handler_probe_value[0], CSRR(CSR_MIP));
    TEST_ASSERT("after handler return, mainline MPRV-U load/store still uses the main permission context",
        post_mprv_access_ok,
        "post_status=%lx post_load=%lx store_read=%lx guard=%lx",
        post_interrupt_mstatus, post_load, post_store_read,
        read64(guard));
    TEST_ASSERT("a supervisor-only page still faults under the restored mainline MPRV-U context and does not inherit handler privilege",
        deny_page_fault_uses_main_mprv,
        "cause=%lx tval=%lx expected=%lx rd=%lx deny=%lx guard=%lx",
        deny_fault_cause, deny_fault_tval, deny_vaddr, deny_load,
        read64(deny_paddr), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_mprv_u_permission_restore_corner);
}

bool ai_interrupt_memory_m_stip_prefetch_hint_no_side_effect_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH2) + 0x780U;
    const uintptr_t store_target = phys_page_base(SWITCH2) + 0x788U;
    const uintptr_t handler_probe = phys_page_base(SCRATCHPAD) + 0x790U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x798U;
    const uint64_t target_seed = 0x696d637072656601ULL;
    const uint64_t store_seed = 0x696d637072656602ULL;
    const uint64_t store_payload = 0x696d637072656603ULL;
    const uint64_t handler_seed = 0x696d637072656604ULL;
    const uint64_t guard_seed = 0x696d6370726566aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t demand_load = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(target, target_seed);
    write64(store_target, store_seed);
    write64(handler_probe, handler_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    prefetch_r(target);
    prefetch_w(store_target);
    const bool prefetches_are_silent =
        !excpt.triggered &&
        read64(target) == target_seed &&
        read64(store_target) == store_seed &&
        read64(guard) == guard_seed;

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    TEST_SETUP_EXCEPT();
    demand_load = read64(target);
    write64(store_target, store_payload);
    fence_iorw_iorw();
    const bool demand_memory_uses_own_request =
        !excpt.triggered &&
        demand_load == target_seed &&
        read64(target) == target_seed &&
        read64(store_target) == store_payload &&
        read64(handler_probe) == handler_seed &&
        read64(guard) == guard_seed;
    const bool interrupt_log_is_independent =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("software prefetch.r/w before STIP is silent and leaves target, store target and guard unchanged",
        prefetches_are_silent,
        "target=%lx store=%lx guard=%lx",
        read64(target), read64(store_target), read64(guard));
    TEST_ASSERT("STIP handler load is logged exactly once from its own probe address and does not consume the prefetch target",
        interrupt_log_is_independent,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx probe=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_probe_value[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("post-interrupt demand load/store uses demand-request payloads rather than stale prefetch or handler data",
        demand_memory_uses_own_request,
        "load=%lx target=%lx store=%lx handler=%lx guard=%lx",
        demand_load, read64(target), read64(store_target),
        read64(handler_probe), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_prefetch_hint_no_side_effect_corner);
}

bool ai_interrupt_memory_m_stip_store_fault_younger_store_owner_corner()
{
    TEST_START();

    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page young_page = SWITCH1;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x1e40U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x1e40U;
    const uintptr_t young_vaddr = hs_page_base(young_page) + 0x1e60U;
    const uintptr_t young_paddr = phys_page_base(young_page) + 0x1e60U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t young_guard = young_paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = old_paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t old_seed = 0x696d6373666f6c64ULL;
    const uint64_t old_denied = 0x696d6373666f6cddULL;
    const uint64_t young_seed = 0x696d6373796f756eULL;
    const uint64_t young_payload = 0x696d6373796f7511ULL;
    const uint64_t old_guard_seed = 0x696d6373666faa01ULL;
    const uint64_t young_guard_seed = 0x696d6373796faa02ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_priv = 0ULL;
    uint64_t old_after_fault = 0ULL;
    uint64_t old_guard_after_fault = 0ULL;
    uint64_t pending_after_fault = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(old_guard, old_guard_seed);
    write64(young_paddr, young_seed);
    write64(young_guard, young_guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(old_vaddr, old_denied);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_priv = excpt.priv;
    pending_after_fault = CSRR(CSR_MIP);
    old_after_fault = read64(old_paddr);
    old_guard_after_fault = read64(old_guard);

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(young_vaddr, young_payload);
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();
    const bool younger_store_uses_own_owner =
        !excpt.triggered &&
        read64(young_paddr) == young_payload &&
        read64(young_guard) == young_guard_seed &&
        read64(old_paddr) == old_seed &&
        read64(old_guard) == old_guard_seed;
    const bool old_fault_isolated =
        fault_priv == PRIV_M &&
        fault_cause == CAUSE_SAF &&
        fault_tval == old_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        old_after_fault == old_seed &&
        old_guard_after_fault == old_guard_seed;
    const bool interrupt_log_isolated =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("older PMP-denied MPRV-U store reports store access fault and leaves its target unchanged while STIP remains pending",
        old_fault_isolated,
        "priv=%lx cause=%lx tval=%lx pending=%lx old=%lx guard=%lx",
        fault_priv, fault_cause, fault_tval, pending_after_fault,
        old_after_fault, old_guard_after_fault);
    TEST_ASSERT("the intervening STIP handler records exactly one independent log store and clears the pending source",
        interrupt_log_isolated,
        "trap_count=%lx cause=%lx mip=%lx handler_log=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("younger valid store after the interrupt does not inherit the older store fault or handler data owner",
        younger_store_uses_own_owner,
        "young=%lx young_guard=%lx old=%lx old_guard=%lx",
        read64(young_paddr), read64(young_guard), read64(old_paddr),
        read64(old_guard));

    TEST_END(ai_interrupt_memory_m_stip_store_fault_younger_store_owner_corner);
}

bool ai_interrupt_memory_hs_stip_store_fault_remap_new_pa_owner_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page new_page = VSRW_GRW;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t vaddr = hs_page_base(va_page) + 0x6e0U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x6e0U;
    const uintptr_t new_paddr = phys_page_base(new_page) + 0x6e0U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x6e0U;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x6e0U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard = new_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard = marker_paddr + sizeof(uint64_t);
    const uint64_t old_seed = 0x696d6373746c6201ULL;
    const uint64_t new_seed = 0x696d6373746c6202ULL;
    const uint64_t fault_payload = 0x696d6373746c62f0ULL;
    const uint64_t new_payload = 0x696d6373746c62f1ULL;
    const uint64_t marker_payload = 0x696d6373746c62f2ULL;
    const uint64_t old_guard_seed = 0x696d6373746c62a1ULL;
    const uint64_t new_guard_seed = 0x696d6373746c62a2ULL;
    const uint64_t marker_guard_seed = 0x696d6373746c62a3ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    uint64_t warmed_old = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t remap_before = 0ULL;
    uint64_t remap_after = 0ULL;
    uint64_t marker_after = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(marker_paddr, 0ULL);
    write64(old_guard, old_guard_seed);
    write64(new_guard, new_guard_seed);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(va_page, old_page,
        PTE_V | PTE_AD | PTE_R, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);

    TEST_SETUP_EXCEPT();
    warmed_old = ai_imc_ld_probe(vaddr, 0ULL);
    sd(vaddr, fault_payload);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    const bool old_store_fault_isolated =
        excpt.triggered &&
        fault_cause == CAUSE_SPF &&
        fault_tval == vaddr &&
        warmed_old == old_seed;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    ai_imc_write_hs_leaf_to_page(va_page, new_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    TEST_SETUP_EXCEPT();
    remap_before = ai_imc_ld_probe(vaddr, 0ULL);
    sd(vaddr, new_payload);
    sd(marker_vaddr, marker_payload);
    fence_iorw_iorw();
    remap_after = ai_imc_ld_probe(vaddr, 0ULL);
    marker_after = ai_imc_ld_probe(marker_vaddr, 0ULL);
    const bool new_store_uses_new_pa =
        !excpt.triggered &&
        remap_before == new_seed &&
        remap_after == new_payload &&
        marker_after == marker_payload;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool old_new_pa_image =
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_payload &&
        read64(marker_paddr) == marker_payload &&
        read64(old_guard) == old_guard_seed &&
        read64(new_guard) == new_guard_seed &&
        read64(marker_guard) == marker_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the first same-VA store faults on the old read-only leaf and leaves the old physical page unchanged",
        old_store_fault_isolated,
        "warmed=%lx cause=%lx tval=%lx expected_tval=%lx old=%lx",
        warmed_old, fault_cause, fault_tval, vaddr, read64(old_paddr));
    TEST_ASSERT("STIP between the killed store fault and remap is logged exactly once and clears before retry",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after remapping the same VA, the new store and marker use the current physical generation only",
        new_store_uses_new_pa && old_new_pa_image,
        "before=%lx after=%lx marker=%lx old_pa=%lx new_pa=%lx guards=%lx,%lx,%lx",
        remap_before, remap_after, marker_after, read64(old_paddr),
        read64(new_paddr), read64(old_guard), read64(new_guard),
        read64(marker_guard));

    TEST_END(ai_interrupt_memory_hs_stip_store_fault_remap_new_pa_owner_corner);
}

bool ai_interrupt_memory_m_stip_cbozero_line_marker_smoke_corner()
{
    TEST_START();

    const uintptr_t block = phys_page_base(SWITCH2) + 0x1e80U;
    const uintptr_t before_guard = block - sizeof(uint64_t);
    const uintptr_t after_guard = block + 64U;
    const uintptr_t marker = block + 128U;
    const uintptr_t marker_guard = marker + sizeof(uint64_t);
    const uint64_t before_guard_seed = 0x696d6363627a6701ULL;
    const uint64_t after_guard_seed = 0x696d6363627a6702ULL;
    const uint64_t marker_value = 0x696d6363627a6d6bULL;
    const uint64_t marker_guard_seed = 0x696d6363627aaa03ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.ZERO smoke target is 64-byte aligned and has independent guard words",
        (block & 63ULL) == 0ULL,
        "block=%lx before=%lx after=%lx", block, before_guard,
        after_guard);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(block + i * sizeof(uint64_t),
            0x63626f7a6c6e0000ULL + i);
    }
    write64(before_guard, before_guard_seed);
    write64(after_guard, after_guard_seed);
    write64(marker, 0ULL);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_cbozero_once(block);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker_value);
    fence_iorw_iorw();

    bool block_zeroed = true;
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        if (read64(block + i * sizeof(uint64_t)) != 0ULL) {
            block_zeroed = false;
        }
    }
    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool line_marker_contract =
        block_zeroed &&
        read64(before_guard) == before_guard_seed &&
        read64(after_guard) == after_guard_seed &&
        read64(marker) == marker_value &&
        read64(marker_guard) == marker_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP after CBO.ZERO is logged exactly once and clears before the post-handler marker store",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("CBO.ZERO zeroes exactly the target line while preserving guards and the post-interrupt marker",
        line_marker_contract,
        "zeroed=%d first=%lx last=%lx before=%lx after=%lx marker=%lx guard=%lx",
        block_zeroed, read64(block), read64(block + 7U * sizeof(uint64_t)),
        read64(before_guard), read64(after_guard), read64(marker),
        read64(marker_guard));

    TEST_END(ai_interrupt_memory_m_stip_cbozero_line_marker_smoke_corner);
}

bool ai_interrupt_memory_m_stip_cbozero_noncanonical_fault_marker_isolation_corner()
{
    TEST_START();

    const enum test_page page = VSRWX_GRWX;
    const uintptr_t block = phys_page_base(SWITCH2) + 0x1d80U;
    const uintptr_t before_guard = block - sizeof(uint64_t);
    const uintptr_t after_guard = block + 64U;
    const uintptr_t marker = block + 128U;
    const uintptr_t marker_guard = marker + sizeof(uint64_t);
    const uintptr_t canonical_vaddr = hs_page_base(page) + 0x1d80U;
    const uintptr_t invalid_vaddr =
        ai_imc_make_noncanonical_vaddr(canonical_vaddr);
    const uintptr_t canonical_paddr = phys_page_base(page) + 0x1d80U;
    const uint64_t fault_sentinel = 0x696d63303534666cULL;
    const uint64_t canonical_seed = 0x696d633035347365ULL;
    const uint64_t marker_value = 0x696d633035346d6bULL;
    const uint64_t before_guard_seed = 0x696d633035346701ULL;
    const uint64_t after_guard_seed = 0x696d633035346702ULL;
    const uint64_t marker_guard_seed = 0x696d6330353467aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_result = fault_sentinel;
    uint64_t fault_priv = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t rounds = 0ULL;
    bool block_zeroed = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.ZERO valid-line target is 64-byte aligned and the invalid VA differs from the canonical probe",
        (block & 63ULL) == 0ULL &&
        invalid_vaddr != canonical_vaddr,
        "block=%lx canonical=%lx invalid=%lx", block, canonical_vaddr,
        invalid_vaddr);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(block + i * sizeof(uint64_t),
            0x63303534626c0000ULL + i);
    }
    write64(before_guard, before_guard_seed);
    write64(after_guard, after_guard_seed);
    write64(marker, 0ULL);
    write64(marker_guard, marker_guard_seed);
    write64(canonical_paddr, canonical_seed);
    fence_iorw_iorw();

    ai_imc_cbozero_once(block);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        if (read64(block + i * sizeof(uint64_t)) != 0ULL) {
            block_zeroed = false;
        }
    }

    goto_priv(PRIV_HS);
    hspt_init();
    sfence_vma();
    TEST_SETUP_EXCEPT();
    fault_result = ai_imc_ld_probe(invalid_vaddr, fault_sentinel);
    fault_priv = excpt.priv;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool invalid_va_fault_isolated =
        (fault_priv == PRIV_M || fault_priv == PRIV_HS) &&
        fault_cause == CAUSE_LPF &&
        fault_tval == invalid_vaddr &&
        fault_result == fault_sentinel &&
        read64(canonical_paddr) == canonical_seed;
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_trap_tval[0] == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool cbo_marker_image_isolated =
        block_zeroed &&
        read64(before_guard) == before_guard_seed &&
        read64(after_guard) == after_guard_seed &&
        read64(marker) == marker_value &&
        read64(marker_guard) == marker_guard_seed &&
        marker_value != ai_imc_handler_log_page[0];

    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("noncanonical HS load adjacent to the CBO window reports load page fault with raw tval and suppresses rd writeback",
        invalid_va_fault_isolated,
        "priv=%lx cause=%lx tval=%lx invalid=%lx result=%lx canonical=%lx",
        fault_priv, fault_cause, fault_tval, invalid_vaddr,
        fault_result, read64(canonical_paddr));
    TEST_ASSERT("STIP after the invalid-VA fault is logged exactly once and does not reuse the fault tval",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx trap_tval=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_tval[0], ai_imc_trap_mip[0],
        ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("valid CBO.ZERO line image, guards, and post-interrupt marker remain disjoint from the invalid-VA fault and handler log",
        cbo_marker_image_isolated,
        "zeroed=%d first=%lx last=%lx before=%lx after=%lx marker=%lx guard=%lx log=%lx",
        block_zeroed, read64(block), read64(block + 7U * sizeof(uint64_t)),
        read64(before_guard), read64(after_guard), read64(marker),
        read64(marker_guard), ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_m_stip_cbozero_noncanonical_fault_marker_isolation_corner);
}

bool ai_interrupt_memory_m_stip_cbozero_partial_store_line_image_corner()
{
    TEST_START();

    const uintptr_t block = phys_page_base(SWITCH2) + 0x1f00U;
    const uintptr_t before_guard = block - sizeof(uint64_t);
    const uintptr_t after_guard = block + 64U;
    const uint64_t before_guard_seed = 0x696d63637a706701ULL;
    const uint64_t after_guard_seed = 0x696d63637a706702ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    bool line_image_ok = true;
    uint64_t observed0 = 0ULL;
    uint64_t observed1 = 0ULL;
    uint64_t observed2 = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.ZERO partial-store target is 64-byte aligned",
        (block & 63ULL) == 0ULL,
        "block=%lx", block);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(block + i * sizeof(uint64_t),
            0x637a7073746c0000ULL + i);
    }
    write64(before_guard, before_guard_seed);
    write64(after_guard, after_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_cbozero_once(block);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    TEST_SETUP_EXCEPT();
    sb(block + 3U, 0xa1U);
    sh(block + 10U, 0xb2c3U);
    sw(block + 20U, 0xd4e5f607U);
    fence_iorw_iorw();
    observed0 = read64(block);
    observed1 = read64(block + 8U);
    observed2 = read64(block + 16U);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        uint8_t expected = 0U;
        if (i == 3ULL) {
            expected = 0xa1U;
        } else if (i == 10ULL) {
            expected = 0xc3U;
        } else if (i == 11ULL) {
            expected = 0xb2U;
        } else if (i == 20ULL) {
            expected = 0x07U;
        } else if (i == 21ULL) {
            expected = 0xf6U;
        } else if (i == 22ULL) {
            expected = 0xe5U;
        } else if (i == 23ULL) {
            expected = 0xd4U;
        }
        if (read8(block + i) != expected) {
            line_image_ok = false;
        }
    }

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool partial_store_after_cbozero_ok =
        !excpt.triggered &&
        line_image_ok &&
        read64(before_guard) == before_guard_seed &&
        read64(after_guard) == after_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP after CBO.ZERO is logged once before the partial-store line image check",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("partial stores after CBO.ZERO preserve only their byte lanes and do not turn the whole line back into stale zero-forward data",
        partial_store_after_cbozero_ok,
        "line_ok=%d w0=%lx w1=%lx w2=%lx before=%lx after=%lx excpt=%d",
        line_image_ok, observed0, observed1, observed2,
        read64(before_guard), read64(after_guard), excpt.triggered);

    TEST_END(ai_interrupt_memory_m_stip_cbozero_partial_store_line_image_corner);
}

bool ai_interrupt_memory_m_stip_cbozero_amo_handler_amo_owner_corner()
{
    TEST_START();

    const uintptr_t block = phys_page_base(SCRATCHPAD) + 0x1e00U;
    const uintptr_t target = block + 3U * sizeof(uint64_t);
    const uintptr_t before_guard = block - sizeof(uint64_t);
    const uintptr_t after_guard = block + 64U;
    const uintptr_t handler_amo = phys_page_base(SCRATCHPAD) + 0x1e80U;
    const uintptr_t handler_guard = handler_amo + sizeof(uint64_t);
    const uint64_t before_guard_seed = 0x696d63637a616701ULL;
    const uint64_t after_guard_seed = 0x696d63637a616702ULL;
    const uint64_t handler_seed = 0x696d63637a616850ULL;
    const uint64_t handler_addend = 0x23ULL;
    const uint64_t handler_guard_seed = 0x696d63637a6168aaULL;
    const uint64_t main_addend = 0x51ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t main_amo_old = 0xffffffffffffffffULL;
    bool target_line_image_ok = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.ZERO plus same-line AMO target is 64-byte aligned and guarded",
        (block & 63ULL) == 0ULL &&
        handler_amo >= block + 64U,
        "block=%lx target=%lx handler=%lx", block, target,
        handler_amo);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(block + i * sizeof(uint64_t),
            0x63627a616d6f0000ULL + i);
    }
    write64(before_guard, before_guard_seed);
    write64(after_guard, after_guard_seed);
    write64(handler_amo, handler_seed);
    write64(handler_guard, handler_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_amo;
    ai_imc_handler_amo_addend = handler_addend;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_amo_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_cbozero_once(block);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    main_amo_old = ai_imc_amoadd_d_aqrl((uint64_t *)target,
        main_addend);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        const uint64_t expected =
            (i == 3ULL) ? main_addend : 0ULL;
        if (read64(block + i * sizeof(uint64_t)) != expected) {
            target_line_image_ok = false;
        }
    }
    const bool interrupt_handler_amo_exact =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_amo_old[0] == handler_seed &&
        ai_imc_handler_amo_final[0] == handler_seed + handler_addend &&
        ai_imc_handler_probe_value[0] == handler_seed + handler_addend &&
        read64(handler_amo) == handler_seed + handler_addend &&
        read64(handler_guard) == handler_guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool cbozero_amo_owner_ok =
        main_amo_old == 0ULL &&
        target_line_image_ok &&
        read64(target) == main_addend &&
        read64(before_guard) == before_guard_seed &&
        read64(after_guard) == after_guard_seed;
    const bool handler_log_not_target_owner =
        ai_imc_handler_amo_final[0] != main_amo_old &&
        ai_imc_handler_amo_final[0] != read64(target) &&
        handler_amo != target &&
        read64(handler_amo) != read64(target);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    ai_imc_handler_amo_addend = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler AMO updates exactly one independent log target after CBO.ZERO and clears source state",
        interrupt_handler_amo_exact,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx handler_old=%lx handler_final=%lx mem=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_amo_old[0],
        ai_imc_handler_amo_final[0], read64(handler_amo),
        ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("mainline AMO old-value comes from the zeroed target line and final line image has one RMW owner",
        cbozero_amo_owner_ok,
        "amo_old=%lx target=%lx line_ok=%d w0=%lx w3=%lx w7=%lx before=%lx after=%lx",
        main_amo_old, read64(target), target_line_image_ok,
        read64(block), read64(target),
        read64(block + 7U * sizeof(uint64_t)), read64(before_guard),
        read64(after_guard));
    TEST_ASSERT("handler AMO log value is not accepted as the CBO.ZERO/AMO target owner",
        handler_log_not_target_owner,
        "handler_final=%lx handler_mem=%lx target=%lx handler_addr=%lx target_addr=%lx",
        ai_imc_handler_amo_final[0], read64(handler_amo),
        read64(target), handler_amo, target);

    TEST_END(ai_interrupt_memory_m_stip_cbozero_amo_handler_amo_owner_corner);
}

bool ai_interrupt_memory_m_stip_misaligned_store_byte_image_corner()
{
    TEST_START();

    static const uint8_t seed_bytes[16] = {
        0xa0U, 0xa1U, 0xa2U, 0xa3U,
        0xa4U, 0xa5U, 0xa6U, 0xa7U,
        0xa8U, 0xa9U, 0xaaU, 0xabU,
        0xacU, 0xadU, 0xaeU, 0xafU,
    };
    static const uint8_t expected_bytes[16] = {
        0xa0U, 0xa1U, 0xa2U, 0x11U,
        0x22U, 0x33U, 0x44U, 0x55U,
        0x66U, 0x77U, 0x88U, 0xabU,
        0xacU, 0xadU, 0xaeU, 0xafU,
    };
    const uintptr_t base = phys_page_base(SWITCH2) + 0x680U;
    const uintptr_t misaligned_addr = base + 3U;
    const uint64_t payload = 0x8877665544332211ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t mip_before_enable = 0ULL;
    uint64_t mip_after_trap = 0ULL;
    uint64_t rounds = 0ULL;
    bool misaligned_store_committed = false;

    goto_priv(PRIV_M);
    ai_imc_write_bytes(base, seed_bytes, sizeof(seed_bytes));
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    sd(misaligned_addr, payload);
    fence_iorw_iorw();
    mip_before_enable = CSRR(CSR_MIP);
    misaligned_store_committed =
        excpt.triggered == false &&
        (mip_before_enable & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_bytes_match(base, expected_bytes, sizeof(expected_bytes));

    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    const bool interrupt_seen =
        rounds < AI_IMC_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("ordinary cacheable misaligned SD commits the exact little-endian byte image while STIP is pending but disabled",
        misaligned_store_committed);
    TEST_ASSERT("after enable, the preserved STIP is delivered once and clears its pending source",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);
    TEST_ASSERT("interrupt handling does not pollute the misaligned-store byte image or adjacent guard bytes",
        ai_imc_bytes_match(base, expected_bytes, sizeof(expected_bytes)));

    TEST_END(ai_interrupt_memory_m_stip_misaligned_store_byte_image_corner);
}

bool ai_interrupt_memory_m_stip_dual_misaligned_store_template_owner_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH2) + 0x760U;
    const uintptr_t first_addr = base + 3U;
    const uintptr_t second_addr = base + 17U;
    const uintptr_t low_guard = base - sizeof(uint64_t);
    const uintptr_t high_guard = base + 32U;
    const uint64_t first_payload = 0x8877665544332211ULL;
    const uint32_t second_payload = 0xddccbbaaU;
    const uint64_t low_guard_seed = 0x696d636d69736701ULL;
    const uint64_t high_guard_seed = 0x696d636d69736702ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint8_t seed[32];
    uint8_t expected_after_first[32];
    uint8_t expected_final[32];
    uint64_t pending_after_first = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    for (uint64_t i = 0ULL; i < sizeof(seed); ++i) {
        seed[i] = (uint8_t)(0x80U + i);
        expected_after_first[i] = seed[i];
        expected_final[i] = seed[i];
    }
    for (uint64_t i = 0ULL; i < sizeof(uint64_t); ++i) {
        expected_after_first[3U + i] =
            (uint8_t)(first_payload >> (i * 8U));
        expected_final[3U + i] =
            (uint8_t)(first_payload >> (i * 8U));
    }
    for (uint64_t i = 0ULL; i < sizeof(uint32_t); ++i) {
        expected_final[17U + i] =
            (uint8_t)((uint64_t)second_payload >> (i * 8U));
    }

    goto_priv(PRIV_M);
    ai_imc_write_bytes(base, seed, sizeof(seed));
    write64(low_guard, low_guard_seed);
    write64(high_guard, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    TEST_SETUP_EXCEPT();
    sd(first_addr, first_payload);
    fence_iorw_iorw();
    const bool first_store_no_fault = !excpt.triggered;
    pending_after_first = CSRR(CSR_MIP);
    const bool first_template_image_ok =
        first_store_no_fault &&
        (pending_after_first & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        ai_imc_bytes_match(base, expected_after_first,
            sizeof(expected_after_first)) &&
        read64(low_guard) == low_guard_seed &&
        read64(high_guard) == high_guard_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_logged_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL;

    TEST_SETUP_EXCEPT();
    sw(second_addr, second_payload);
    fence_iorw_iorw();
    const bool second_store_no_fault = !excpt.triggered;
    const bool second_template_owner_ok =
        second_store_no_fault &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_bytes_match(base, expected_final, sizeof(expected_final)) &&
        read64(low_guard) == low_guard_seed &&
        read64(high_guard) == high_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("first misaligned SD commits its own byte template while STIP remains pending and disabled",
        first_template_image_ok,
        "first_ok=%d pending=%lx traps=%lx base3=%x base10=%x guards=%lx,%lx",
        first_store_no_fault, pending_after_first, ai_imc_trap_count,
        read8(base + 3U), read8(base + 10U), read64(low_guard),
        read64(high_guard));
    TEST_ASSERT("the intervening STIP handler logs exactly once and clears pending before the second misaligned store",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("post-handler misaligned SW uses its own offset/width template and does not replay the older SD mask or payload",
        second_template_owner_ok,
        "second_ok=%d base17=%x base20=%x base21=%x traps=%lx guards=%lx,%lx",
        second_store_no_fault, read8(base + 17U), read8(base + 20U),
        read8(base + 21U), ai_imc_trap_count, read64(low_guard),
        read64(high_guard));

    TEST_END(ai_interrupt_memory_m_stip_dual_misaligned_store_template_owner_corner);
}

bool ai_interrupt_memory_hs_stip_scalar_cross_page_split_owner_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t split_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint32_t);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t low_guard_paddr = low_paddr - sizeof(uint64_t);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t low_guard_seed = 0x696d6373706c6701ULL;
    const uint64_t high_guard_seed = 0x696d637370686702ULL;
    const uint64_t load_low_seed = 0x1021324354657687ULL;
    const uint64_t load_high_seed = 0x98a9bacbdcedfe0fULL;
    const uint64_t store_low_seed = 0x21436587a9cbed0fULL;
    const uint64_t store_high_seed = 0xfedcba9876543210ULL;
    const uint64_t store_fault_payload = 0x0123456789abcdefULL;
    const uint64_t store_retry_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t expected_load =
        ((load_high_seed & 0xffffffffULL) << 32U) |
        (load_low_seed >> 32U);
    const uint64_t expected_retry_low =
        (store_low_seed & 0xffffffffULL) |
        ((store_retry_payload & 0xffffffffULL) << 32U);
    const uint64_t expected_retry_high =
        (store_high_seed & 0xffffffff00000000ULL) |
        (store_retry_payload >> 32U);
    const uint64_t allowed_fault_prefix_low =
        (store_low_seed & 0xffffffffULL) |
        ((store_fault_payload & 0xffffffffULL) << 32U);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t loaded = 0ULL;
    uint64_t store_fault_cause = 0ULL;
    uint64_t store_fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_paddr, load_low_seed);
    write64(high_paddr, load_high_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, legal_leaf);
    ai_imc_set_hs_leaf(high_page, legal_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    loaded = ld(split_vaddr);
    const bool fault_free_load_stitches_halves =
        !excpt.triggered &&
        loaded == expected_load &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_paddr, store_low_seed);
    write64(high_paddr, store_high_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(low_page, legal_leaf);
    ai_imc_set_hs_leaf(high_page, no_write_leaf);
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, store_fault_payload);
    const bool store_fault_triggered = excpt.triggered;
    store_fault_cause = excpt.cause;
    store_fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool low_half_fault_prefix_allowed =
        read64(low_paddr) == store_low_seed ||
        read64(low_paddr) == allowed_fault_prefix_low;
    const bool high_half_fault_contract =
        store_fault_triggered &&
        store_fault_cause == CAUSE_SPF &&
        store_fault_tval == second_half_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        read64(low_guard_paddr) == low_guard_seed &&
        low_half_fault_prefix_allowed &&
        read64(high_paddr) == store_high_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_after_fault_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, legal_leaf);
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, store_retry_payload);
    const bool retry_store_no_fault = !excpt.triggered;

    goto_priv(PRIV_M);
    const bool retry_uses_fresh_low_high_halves =
        retry_store_no_fault &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(low_paddr) == expected_retry_low &&
        read64(low_paddr) != allowed_fault_prefix_low &&
        read64(high_paddr) == expected_retry_high &&
        read64(high_paddr) !=
            ((store_high_seed & 0xffffffff00000000ULL) |
            (store_fault_payload >> 32U)) &&
        read64(high_guard_paddr) == high_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("fault-free scalar cross-page LD stitches the low and high halves from the current page pair",
        fault_free_load_stitches_halves,
        "loaded=%lx expected=%lx low=%lx high=%lx guards=%lx,%lx",
        loaded, expected_load, read64(low_paddr), read64(high_paddr),
        read64(low_guard_paddr), read64(high_guard_paddr));
    TEST_ASSERT("cross-page SD high-half page fault reports the high-page tval, permits only a legal low-half prefix, and leaves STIP pending",
        high_half_fault_contract,
        "cause=%lx tval=%lx expected=%lx pending=%lx low=%lx old_low=%lx prefix_low=%lx high=%lx traps=%lx",
        store_fault_cause, store_fault_tval, second_half_vaddr,
        pending_after_fault, read64(low_paddr), store_low_seed,
        allowed_fault_prefix_low, read64(high_paddr), ai_imc_trap_count);
    TEST_ASSERT("the preserved STIP after the split fault is delivered exactly once and clears pending state",
        interrupt_after_fault_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("after high-page repair, retry SD uses one fresh payload for both low and high halves without mixing the old fault payload",
        retry_uses_fresh_low_high_halves,
        "retry_ok=%d low=%lx expected_low=%lx high=%lx expected_high=%lx guards=%lx,%lx",
        retry_store_no_fault, read64(low_paddr), expected_retry_low,
        read64(high_paddr), expected_retry_high, read64(low_guard_paddr),
        read64(high_guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_scalar_cross_page_split_owner_corner);
}

bool ai_interrupt_memory_hs_stip_misaligned_store_fault_handler_line_image_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const uintptr_t split_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t low_guard_paddr = low_paddr - sizeof(uint64_t);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uintptr_t handler_lane_paddr = low_paddr + 1U;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t low_seed = 0x21436587a9cbed0fULL;
    const uint64_t high_seed = 0xfedcba9876543210ULL;
    const uint64_t fault_payload = 0x0123456789abcdefULL;
    const uint64_t retry_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t low_guard_seed = 0x696d636d736c6701ULL;
    const uint64_t high_guard_seed = 0x696d636d73686702ULL;
    const uint8_t handler_byte = 0x5aU;
    const uint64_t low_after_fault_prefix =
        (low_seed & 0xffffffffULL) |
        ((fault_payload & 0xffffffffULL) << 32U);
    const uint64_t high_after_retry =
        (high_seed & 0xffffffff00000000ULL) |
        (retry_payload >> 32U);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t low_after_fault = 0ULL;
    uint64_t low_after_handler = 0ULL;
    uint64_t low_after_retry = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;
    bool store_fault_triggered = false;
    bool retry_triggered = false;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, legal_leaf);
    ai_imc_set_hs_leaf(high_page, no_write_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, fault_payload);
    store_fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    low_after_fault = read64(low_paddr);
    pending_after_fault = CSRR(CSR_MIP);
    const bool fault_event_set_ok =
        store_fault_triggered &&
        fault_cause == CAUSE_SPF &&
        fault_tval == second_half_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        (low_after_fault == low_seed ||
         low_after_fault == low_after_fault_prefix) &&
        read64(high_paddr) == high_seed &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    low_after_handler =
        (low_after_fault & ~0x000000000000ff00ULL) |
        ((uint64_t)handler_byte << 8U);
    low_after_retry = (low_after_handler & 0xffffffffULL) |
        ((retry_payload & 0xffffffffULL) << 32U);
    ai_imc_reset_trap_log();
    ai_imc_handler_store_addr = handler_lane_paddr;
    ai_imc_handler_store_value = handler_byte;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool handler_same_line_lane_ok =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(low_paddr) == low_after_handler &&
        read64(high_paddr) == high_seed &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, legal_leaf);
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, retry_payload);
    retry_triggered = excpt.triggered;
    fence_iorw_iorw();

    goto_priv(PRIV_M);
    const bool retry_image_explains_all_events =
        !retry_triggered &&
        read64(low_paddr) == low_after_retry &&
        read64(high_paddr) == high_after_retry &&
        read8(handler_lane_paddr) == handler_byte &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("cross-page misaligned SD reports the high-page store fault while pending STIP is preserved and the low half remains in a legal event set",
        fault_event_set_ok,
        "cause=%lx tval=%lx expected=%lx pending=%lx low=%lx allowed=%lx/%lx high=%lx guards=%lx,%lx",
        fault_cause, fault_tval, second_half_vaddr, pending_after_fault,
        low_after_fault, low_seed, low_after_fault_prefix,
        read64(high_paddr), read64(low_guard_paddr),
        read64(high_guard_paddr));
    TEST_ASSERT("STIP handler writes exactly one same-line byte lane and does not masquerade as the misaligned store high half",
        handler_same_line_lane_ok,
        "rounds=%lx trap_count=%lx cause=%lx low=%lx expected=%lx high=%lx mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        read64(low_paddr), low_after_handler, read64(high_paddr),
        CSRR(CSR_MIP));
    TEST_ASSERT("after high-page repair, retry SD explains the final byte image as handler lane plus retry low/high halves with no duplicate low-half leak",
        retry_image_explains_all_events,
        "retry_exc=%d low=%lx expected_low=%lx high=%lx expected_high=%lx lane=%x guards=%lx,%lx",
        retry_triggered, read64(low_paddr), low_after_retry,
        read64(high_paddr), high_after_retry, read8(handler_lane_paddr),
        read64(low_guard_paddr), read64(high_guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_misaligned_store_fault_handler_line_image_corner);
}

bool ai_interrupt_memory_hs_stip_misaligned_load_fault_handler_line_merge_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSR_GX;
    const uintptr_t split_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t low_guard_paddr = low_paddr - sizeof(uint64_t);
    const uintptr_t high_guard_paddr = high_paddr + sizeof(uint64_t);
    const uintptr_t handler_lane_paddr = low_paddr + 5U;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t low_seed = 0x1021324354657687ULL;
    const uint64_t high_seed = 0x98a9bacbdcedfe0fULL;
    const uint64_t low_guard_seed = 0x696d636d6c6c6701ULL;
    const uint64_t high_guard_seed = 0x696d636d6c686702ULL;
    const uint8_t handler_byte = 0x6bU;
    const uint64_t rd_sentinel = 0x696d636d6c7264ddULL;
    const uint64_t low_after_handler =
        (low_seed & ~0x0000ff0000000000ULL) |
        ((uint64_t)handler_byte << 40U);
    const uint64_t expected_retry_load =
        ((high_seed & 0xffffffffULL) << 32U) |
        ((low_after_handler >> 32U) & 0xffffffffULL);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t low_half_before_fault = 0ULL;
    uint64_t high_fault_result = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;
    uint64_t retry_low = 0ULL;
    uint64_t retry_high = 0ULL;
    uint64_t retry_load = 0ULL;
    bool low_half_triggered = false;
    bool high_fault_triggered = false;
    bool retry_low_triggered = false;
    bool retry_high_triggered = false;

    goto_priv(PRIV_M);
    write64(low_guard_paddr, low_guard_seed);
    write64(low_paddr, low_seed);
    write64(high_paddr, high_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, legal_leaf);
    ai_imc_set_hs_leaf(high_page, no_read_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    low_half_before_fault = lwu(split_vaddr);
    low_half_triggered = excpt.triggered;

    TEST_SETUP_EXCEPT();
    high_fault_result = ai_imc_lwu_probe(second_half_vaddr, rd_sentinel);
    high_fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool load_fault_contract =
        !low_half_triggered &&
        low_half_before_fault == ((low_seed >> 32U) & 0xffffffffULL) &&
        high_fault_triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == second_half_vaddr &&
        high_fault_result == rd_sentinel &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        read64(low_paddr) == low_seed &&
        read64(high_paddr) == high_seed &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    ai_imc_reset_trap_log();
    ai_imc_handler_store_addr = handler_lane_paddr;
    ai_imc_handler_store_value = handler_byte;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool handler_store_is_own_event =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(low_paddr) == low_after_handler &&
        read64(high_paddr) == high_seed &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, legal_leaf);
    TEST_SETUP_EXCEPT();
    retry_low = lwu(split_vaddr);
    retry_low_triggered = excpt.triggered;
    TEST_SETUP_EXCEPT();
    retry_high = lwu(second_half_vaddr);
    retry_high_triggered = excpt.triggered;
    retry_load = (retry_high << 32U) | retry_low;

    goto_priv(PRIV_M);
    const bool retry_reloads_full_image =
        !retry_low_triggered &&
        !retry_high_triggered &&
        retry_load == expected_retry_load &&
        retry_low == ((low_after_handler >> 32U) & 0xffffffffULL) &&
        retry_high == (high_seed & 0xffffffffULL) &&
        read64(low_paddr) == low_after_handler &&
        read64(high_paddr) == high_seed &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("staged split-load baseline reads the low half then reports the high-page load fault while preserving rd sentinel, memory image and pending STIP",
        load_fault_contract,
        "low_half=%lx low_exc=%d cause=%lx tval=%lx expected=%lx high_rd=%lx sentinel=%lx pending=%lx low=%lx high=%lx",
        low_half_before_fault, low_half_triggered, fault_cause,
        fault_tval, second_half_vaddr, high_fault_result, rd_sentinel,
        pending_after_fault, read64(low_paddr), read64(high_paddr));
    TEST_ASSERT("STIP handler writes one same-line byte as its own event after the load fault and keeps source clearing independent",
        handler_store_is_own_event,
        "rounds=%lx trap_count=%lx cause=%lx low=%lx expected_low=%lx high=%lx mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        read64(low_paddr), low_after_handler, read64(high_paddr),
        CSRR(CSR_MIP));
    TEST_ASSERT("after high-page repair, the staged retry reloads low and high halves including the handler byte without stale merge",
        retry_reloads_full_image,
        "retry_exc=%d/%d retry_low=%lx retry_high=%lx load=%lx expected=%lx low=%lx high=%lx guards=%lx,%lx",
        retry_low_triggered, retry_high_triggered, retry_low, retry_high,
        retry_load, expected_retry_load,
        read64(low_paddr), read64(high_paddr), read64(low_guard_paddr),
        read64(high_guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_misaligned_load_fault_handler_line_merge_corner);
}

bool ai_interrupt_memory_m_stip_amo_aqrl_ordering_marker_corner()
{
    TEST_START();

    const uintptr_t before_marker = phys_page_base(SCRATCHPAD) + 0x700U;
    const uintptr_t amo_addr = phys_page_base(SCRATCHPAD) + 0x708U;
    const uintptr_t after_marker = phys_page_base(SCRATCHPAD) + 0x710U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x718U;
    const uint64_t before_value = 0x696d636171726c01ULL;
    const uint64_t amo_seed = 0x696d636171726c02ULL;
    const uint64_t amo_addend = 0x31ULL;
    const uint64_t after_value = 0x696d636171726c03ULL;
    const uint64_t guard_seed = 0x696d63617172aa04ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t amo_old = 0ULL;
    uint64_t before_observed = 0ULL;
    uint64_t final_observed = 0ULL;
    uint64_t after_observed = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    goto_priv(PRIV_M);
    write64(before_marker, 0ULL);
    write64(amo_addr, amo_seed);
    write64(after_marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    TEST_SETUP_EXCEPT();
    write64(before_marker, before_value);
    amo_old = ai_imc_amoadd_d_aqrl((uint64_t *)amo_addr, amo_addend);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    write64(after_marker, after_value);
    fence_iorw_iorw();
    before_observed = read64(before_marker);
    final_observed = read64(amo_addr);
    after_observed = read64(after_marker);

    const bool interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_STI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("AMO.AQRL reports the old target value and updates memory exactly once around the interrupt window",
        amo_old == amo_seed &&
        final_observed == amo_seed + amo_addend &&
        read64(amo_addr) == amo_seed + amo_addend);
    TEST_ASSERT("ordering markers before and after the AMO remain separate from the interrupt handler path",
        before_observed == before_value &&
        after_observed == after_value &&
        read64(guard) == guard_seed);
    TEST_ASSERT("STIP adjacent to the AMO.AQRL ordering window is delivered once and clears its pending source",
        interrupt_seen &&
        (mip_after_trap & AI_IMC_MIP_STIP) == 0ULL);

    TEST_END(ai_interrupt_memory_m_stip_amo_aqrl_ordering_marker_corner);
}

bool ai_interrupt_memory_m_stip_amo_aqrl_handler_log_order_marker_corner()
{
    TEST_START();

    const uintptr_t release_marker = phys_page_base(SWITCH1) + 0xa00U;
    const uintptr_t amo_addr = phys_page_base(SWITCH1) + 0xa08U;
    const uintptr_t acquire_marker = phys_page_base(SWITCH1) + 0xa10U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xa18U;
    const uint64_t release_value = 0x696d636171686101ULL;
    const uint64_t amo_seed = 0x696d636171686102ULL;
    const uint64_t amo_addend = 0x42ULL;
    const uint64_t acquire_value = 0x696d636171686103ULL;
    const uint64_t guard_seed = 0x696d63617168aa04ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t amo_old = 0ULL;
    uint64_t release_after_handler = 0ULL;
    uint64_t target_after_handler = 0ULL;
    uint64_t acquire_before_main_write = 0ULL;
    uint64_t handler_log_snapshot = 0ULL;

    goto_priv(PRIV_M);
    write64(release_marker, 0ULL);
    write64(amo_addr, amo_seed);
    write64(acquire_marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(release_marker, release_value);
    amo_old = ai_imc_amoadd_d_aqrl((uint64_t *)amo_addr, amo_addend);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    release_after_handler = read64(release_marker);
    target_after_handler = read64(amo_addr);
    acquire_before_main_write = read64(acquire_marker);
    handler_log_snapshot = ai_imc_handler_log_page[0];
    write64(acquire_marker, acquire_value);
    fence_iorw_iorw();

    const bool interrupt_log_exact =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        handler_log_snapshot == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool amo_owner_ok =
        amo_old == amo_seed &&
        target_after_handler == amo_seed + amo_addend &&
        read64(amo_addr) == amo_seed + amo_addend;
    const bool marker_order_ok =
        release_after_handler == release_value &&
        acquire_before_main_write == 0ULL &&
        read64(acquire_marker) == acquire_value &&
        read64(guard) == guard_seed;
    const bool handler_log_not_main_witness =
        read64(release_marker) != handler_log_snapshot &&
        read64(acquire_marker) != handler_log_snapshot &&
        read64(amo_addr) != handler_log_snapshot &&
        release_marker != (uintptr_t)&ai_imc_handler_log_page[0] &&
        amo_addr != (uintptr_t)&ai_imc_handler_log_page[0] &&
        acquire_marker != (uintptr_t)&ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between AMO.AQRL and the acquire marker writes exactly one independent handler log and clears source state",
        interrupt_log_exact,
        "trap_count=%lx cause=%lx mip_log=%lx handler_log=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], handler_log_snapshot, CSRR(CSR_MIP));
    TEST_ASSERT("AMO.AQRL old value and final memory stay bound to the mainline target, not the handler log",
        amo_owner_ok,
        "old=%lx target_after_handler=%lx target_final=%lx expected=%lx",
        amo_old, target_after_handler, read64(amo_addr),
        amo_seed + amo_addend);
    TEST_ASSERT("handler log cannot substitute for the release/acquire memory markers around the AMO window",
        marker_order_ok && handler_log_not_main_witness,
        "release_after=%lx acquire_before=%lx acquire_final=%lx log=%lx guard=%lx",
        release_after_handler, acquire_before_main_write,
        read64(acquire_marker), handler_log_snapshot, read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_amo_aqrl_handler_log_order_marker_corner);
}

bool ai_interrupt_memory_m_stip_handler_log_target_isolation_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH1) + 0x720U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x728U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x730U;
    const uint64_t target_seed = 0x696d6369736f0101ULL;
    const uint64_t target_value = 0x696d6369736f0111ULL;
    const uint64_t guard_seed = 0x696d6369736faa02ULL;
    const uint64_t marker_value = 0x696d6369736f0122ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(target, target_seed);
    write64(guard, guard_seed);
    write64(marker, 0ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    write64(target, target_value);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool handler_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool mainline_memory_isolated =
        read64(target) == target_value &&
        read64(guard) == guard_seed &&
        read64(marker) == marker_value &&
        target != (uintptr_t)&ai_imc_handler_log_page[0] &&
        guard != (uintptr_t)&ai_imc_handler_log_page[0] &&
        marker != (uintptr_t)&ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("custom interrupt handler writes exactly one independent log slot and clears STIP",
        handler_logged_once);
    TEST_ASSERT("handler scratch/log memory remains disjoint from testcase target, guard, and marker regions",
        mainline_memory_isolated);
    TEST_ASSERT("mainline target and marker retain their exactly-once values after handler return",
        read64(target) == target_value &&
        read64(guard) == guard_seed &&
        read64(marker) == marker_value);

    TEST_END(ai_interrupt_memory_m_stip_handler_log_target_isolation_corner);
}

bool ai_interrupt_memory_m_stip_fault_interrupt_trap_csr_isolation_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x760U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x760U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d636374637201ULL;
    const uint64_t guard_seed = 0x696d63637463aa02ULL;
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
    uint64_t fault_first_cause = 0ULL;
    uint64_t fault_first_tval = 0ULL;
    uint64_t fault_first_priv = 0ULL;
    uint64_t interrupt_first_cause = 0ULL;
    uint64_t fault_after_interrupt_cause = 0ULL;
    uint64_t fault_after_interrupt_tval = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_R);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_imc_ld_probe(vaddr, 0ULL);
    ai_imc_clear_mprv_translation();
    fault_first_cause = excpt.cause;
    fault_first_tval = excpt.tval;
    fault_first_priv = excpt.priv;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    interrupt_first_cause =
        ai_imc_trap_count > 0ULL ? ai_imc_trap_cause[0] : 0ULL;

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_imc_ld_probe(vaddr, 0ULL);
    ai_imc_clear_mprv_translation();
    fault_after_interrupt_cause = excpt.cause;
    fault_after_interrupt_tval = excpt.tval;

    const bool fault_first_log_stable =
        fault_first_priv == PRIV_M &&
        fault_first_cause == CAUSE_LAF &&
        fault_first_tval == vaddr &&
        interrupt_first_cause == CAUSE_STI &&
        rounds < AI_IMC_SPIN_LIMIT;
    const bool interrupt_first_then_fault_log_stable =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        fault_after_interrupt_cause == CAUSE_LAF &&
        fault_after_interrupt_tval == vaddr;
    const bool memory_unchanged =
        read64(paddr) == seed &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("fault-first back-to-back sequence keeps load access fault cause/tval separate from later STIP cause",
        fault_first_log_stable);
    TEST_ASSERT("interrupt-first back-to-back sequence logs STIP separately and the following fault refreshes cause/tval",
        interrupt_first_then_fault_log_stable,
        "trap_count=%lx trap0_cause=%lx fault_cause=%lx fault_tval=%lx vaddr=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        fault_after_interrupt_cause, fault_after_interrupt_tval, vaddr);
    TEST_ASSERT("back-to-back fault/interrupt logging does not change target or guard memory",
        memory_unchanged);

    TEST_END(ai_interrupt_memory_m_stip_fault_interrupt_trap_csr_isolation_corner);
}

bool ai_interrupt_memory_m_stip_pte_ad_fault_retry_generation_corner()
{
    TEST_START();

    const enum test_page load_page = VSR_GR;
    const enum test_page store_page = VSRW_GRW;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(load_page) + 0x780U;
    const uintptr_t load_paddr = phys_page_base(load_page) + 0x780U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x788U;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x788U;
    const uintptr_t guard = phys_page_base(guard_page) + 0x790U;
    const uint64_t load_value = 0x696d6361646c6401ULL;
    const uint64_t store_seed = 0x696d636164737430ULL;
    const uint64_t store_payload = 0x696d636164737431ULL;
    const uint64_t denied_payload = 0x696d63616464656eULL;
    const uint64_t guard_seed = 0x696d636164677501ULL;
    const uint64_t load_sentinel = 0x696d63616473656eULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    pte_t *load_leaf = NULL;
    pte_t *store_leaf = NULL;
    pte_t load_a0_leaf = 0ULL;
    pte_t store_d0_leaf = 0ULL;
    pte_t load_a1_leaf = 0ULL;
    pte_t store_d1_leaf = 0ULL;
    uint64_t denied_load = 0ULL;
    uint64_t load_fault_cause = 0ULL;
    uint64_t load_fault_tval = 0ULL;
    uint64_t store_fault_cause = 0ULL;
    uint64_t store_fault_tval = 0ULL;
    uint64_t interrupt_cause = 0ULL;
    uint64_t retry_load = 0ULL;
    uint64_t retry_store_read = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(load_paddr, load_value);
    write64(store_paddr, store_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    goto_priv(PRIV_M);
    load_leaf = ai_imc_hs_leaf_entry(load_page);
    store_leaf = ai_imc_hs_leaf_entry(store_page);
    *load_leaf = (phys_page_base(load_page) >> 2U) |
        PTE_V | PTE_D | PTE_R;
    *store_leaf = (phys_page_base(store_page) >> 2U) |
        PTE_V | PTE_A | PTE_RW;
    load_a0_leaf = *load_leaf;
    store_d0_leaf = *store_leaf;
    sfence_vma();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    denied_load = ai_imc_ld_probe(load_vaddr, load_sentinel);
    load_fault_cause = excpt.cause;
    load_fault_tval = excpt.tval;

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, denied_payload);
    store_fault_cause = excpt.cause;
    store_fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    const bool ad_faults_do_not_half_update =
        load_fault_cause == CAUSE_LPF &&
        load_fault_tval == load_vaddr &&
        denied_load == load_sentinel &&
        store_fault_cause == CAUSE_SPF &&
        store_fault_tval == store_vaddr &&
        *load_leaf == load_a0_leaf &&
        *store_leaf == store_d0_leaf &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_seed &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) != 0ULL;

    *load_leaf = (phys_page_base(load_page) >> 2U) |
        PTE_V | PTE_AD | PTE_R;
    *store_leaf = (phys_page_base(store_page) >> 2U) |
        PTE_V | PTE_AD | PTE_RW;
    load_a1_leaf = *load_leaf;
    store_d1_leaf = *store_leaf;
    sfence_vma();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    interrupt_cause = ai_imc_trap_count > 0ULL ?
        ai_imc_trap_cause[0] : 0ULL;
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    retry_load = ld(load_vaddr);
    const bool retry_load_ok =
        !excpt.triggered && retry_load == load_value;

    TEST_SETUP_EXCEPT();
    sd(store_vaddr, store_payload);
    const bool retry_store_ok = !excpt.triggered;

    TEST_SETUP_EXCEPT();
    retry_store_read = ld(store_vaddr);
    const bool retry_read_ok =
        !excpt.triggered && retry_store_read == store_payload;

    goto_priv(PRIV_M);
    const bool interrupt_and_retry_state_stable =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        interrupt_cause == CAUSE_STI &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        retry_load_ok &&
        retry_store_ok &&
        retry_read_ok &&
        *load_leaf == load_a1_leaf &&
        *store_leaf == store_d1_leaf &&
        read64(load_paddr) == load_value &&
        read64(store_paddr) == store_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("A=0 load and D=0 store faults keep PTE words, target data, guard data, and pending STIP separate",
        ad_faults_do_not_half_update);
    TEST_ASSERT("after repairing A/D bits and taking the pending STIP, retry load/store use the repaired PTE generation exactly once",
        interrupt_and_retry_state_stable,
        "trap_count=%lx cause=%lx load=%lx store=%lx guard=%lx",
        ai_imc_trap_count, interrupt_cause,
        read64(load_paddr), read64(store_paddr), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_pte_ad_fault_retry_generation_corner);
}

bool ai_interrupt_memory_m_stip_pmp_reconfig_deny_allow_retry_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x7a0U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x7a0U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d63706d707301ULL;
    const uint64_t baseline_payload = 0x696d63706d706201ULL;
    const uint64_t denied_payload = 0x696d63706d706466ULL;
    const uint64_t allowed_payload = 0x696d63706d70616cULL;
    const uint64_t guard_seed = 0x696d63706d70aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t deny_fault_cause = 0ULL;
    uint64_t deny_fault_tval = 0ULL;
    uint64_t pmp_deny_cfg = 0ULL;
    uint64_t pmp_allow_cfg = 0ULL;
    uint64_t interrupt_cause = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, baseline_payload);
    ai_imc_clear_mprv_translation();
    const bool baseline_access_ok =
        !excpt.triggered &&
        read64(paddr) == baseline_payload &&
        read64(guard) == guard_seed;

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);
    pmp_deny_cfg = CSRR(CSR_PMPCFG0);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    ai_imc_clear_mprv_translation();
    deny_fault_cause = excpt.cause;
    deny_fault_tval = excpt.tval;

    const bool deny_generation_blocks_store =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        deny_fault_cause == CAUSE_SAF &&
        deny_fault_tval == vaddr &&
        pmp_deny_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_NO_W) &&
        read64(paddr) == baseline_payload &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) != 0ULL;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    interrupt_cause = ai_imc_trap_count > 0ULL ?
        ai_imc_trap_cause[0] : 0ULL;
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    pmp_allow_cfg = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allowed_payload);
    ai_imc_clear_mprv_translation();
    const bool allow_generation_commits_once =
        !excpt.triggered &&
        pmp_allow_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_RWX) &&
        read64(paddr) == allowed_payload &&
        read64(guard) == guard_seed;

    const bool interrupt_did_not_corrupt_pmp_or_memory =
        baseline_access_ok &&
        deny_generation_blocks_store &&
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        interrupt_cause == CAUSE_STI &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        allow_generation_commits_once;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("baseline translated store proves the selected 4KB PMP window is reachable before deny reconfiguration",
        baseline_access_ok);
    TEST_ASSERT("PMP deny generation plus pending STIP produces a store access fault and no side effect",
        deny_generation_blocks_store,
        "cause=%lx tval=%lx cfg=%lx target=%lx guard=%lx",
        deny_fault_cause, deny_fault_tval, pmp_deny_cfg,
        read64(paddr), read64(guard));
    TEST_ASSERT("after STIP is handled and PMP is reconfigured to allow, the retry store commits exactly once through the final permission",
        interrupt_did_not_corrupt_pmp_or_memory,
        "trap_count=%lx cause=%lx cfg=%lx target=%lx guard=%lx",
        ai_imc_trap_count, interrupt_cause, pmp_allow_cfg,
        read64(paddr), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_pmp_reconfig_deny_allow_retry_corner);
}

bool ai_interrupt_memory_hs_ssip_sum_mxr_permission_restore_corner()
{
    TEST_START();

    const enum test_page sum_page = VSURWX_GRWX;
    const enum test_page mxr_page = VSX_GRWX;
    const uintptr_t sum_vaddr = hs_page_base(sum_page) + 0x7c0U;
    const uintptr_t sum_paddr = phys_page_base(sum_page) + 0x7c0U;
    const uintptr_t mxr_vaddr = hs_page_base(mxr_page) + 0x7c8U;
    const uintptr_t mxr_paddr = phys_page_base(mxr_page) + 0x7c8U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x7d0U;
    const uint64_t sum_value = 0x696d6373756d0101ULL;
    const uint64_t mxr_value = 0x696d636d78720102ULL;
    const uint64_t guard_seed = 0x696d63736d78aa03ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t sum_loaded = 0ULL;
    uint64_t mxr_loaded = 0ULL;
    uint64_t sum_sstatus_after_interrupt = 0ULL;
    uint64_t mxr_sstatus_after_interrupt = 0ULL;
    uint64_t sum_sip_after_interrupt = 0ULL;
    uint64_t mxr_sip_after_interrupt = 0ULL;

    goto_priv(PRIV_M);
    write64(sum_paddr, sum_value);
    write64(mxr_paddr, mxr_value);
    write64(guard, guard_seed);
    hspt_init();
    sfence_vma();

    ai_imc_prepare_hs_ssip(0ULL);
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT | SSTATUS_MXR);
    CSRS(CSR_SSTATUS, SSTATUS_SUM);
    CSRS(CSR_SIP, AI_IMC_SIP_SSIP);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    sum_sstatus_after_interrupt = CSRR(CSR_SSTATUS);
    sum_sip_after_interrupt = CSRR(CSR_SIP);
    const bool sum_interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SSI;

    TEST_SETUP_EXCEPT();
    sum_loaded = ld(sum_vaddr);
    const bool sum_load_allowed_after_interrupt =
        !excpt.triggered &&
        sum_loaded == sum_value &&
        (sum_sstatus_after_interrupt & SSTATUS_SUM) != 0ULL &&
        (sum_sstatus_after_interrupt & SSTATUS_MXR) == 0ULL &&
        (sum_sip_after_interrupt & AI_IMC_SIP_SSIP) == 0ULL;

    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT | SSTATUS_SUM);
    CSRS(CSR_SSTATUS, SSTATUS_MXR);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRS(CSR_SIP, AI_IMC_SIP_SSIP);
    TEST_SETUP_EXCEPT();
    CSRS(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    (void)ai_imc_spin_until_trap(AI_IMC_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    mxr_sstatus_after_interrupt = CSRR(CSR_SSTATUS);
    mxr_sip_after_interrupt = CSRR(CSR_SIP);
    const bool mxr_interrupt_seen =
        excpt.triggered &&
        excpt.priv == PRIV_HS &&
        excpt.cause == CAUSE_SSI;

    TEST_SETUP_EXCEPT();
    mxr_loaded = ld(mxr_vaddr);
    const bool mxr_load_allowed_after_interrupt =
        !excpt.triggered &&
        mxr_loaded == mxr_value &&
        (mxr_sstatus_after_interrupt & SSTATUS_SUM) == 0ULL &&
        (mxr_sstatus_after_interrupt & SSTATUS_MXR) != 0ULL &&
        (mxr_sip_after_interrupt & AI_IMC_SIP_SSIP) == 0ULL;

    goto_priv(PRIV_M);
    const bool guard_unchanged =
        read64(sum_paddr) == sum_value &&
        read64(mxr_paddr) == mxr_value &&
        read64(guard) == guard_seed;

    ai_imc_restore_s_interrupt_state(saved_sstatus, saved_sie);
    goto_priv(PRIV_M);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("delegated SSIP is delivered in HS-as-S mode and preserves the SUM=1 permission window",
        sum_interrupt_seen &&
        sum_load_allowed_after_interrupt);
    TEST_ASSERT("a second delegated SSIP preserves MXR=1 so the execute-only supervisor leaf remains readable",
        mxr_interrupt_seen &&
        mxr_load_allowed_after_interrupt);
    TEST_ASSERT("SUM/MXR interrupt windows do not corrupt the backing memory image or independent guard",
        guard_unchanged);

    TEST_END(ai_interrupt_memory_hs_ssip_sum_mxr_permission_restore_corner);
}

bool ai_interrupt_memory_m_ssip_stip_fault_retry_priority_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x7e0U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x7e0U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d636d756c7401ULL;
    const uint64_t retry_payload = 0x696d636d756c7411ULL;
    const uint64_t guard_seed = 0x696d636d756caa02ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t retry_read = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    ai_imc_reset_trap_log();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    CSRC(CSR_MIDELEG, AI_IMC_SIP_SSIP);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_SIP_SSIP | AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_R);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    (void)ai_imc_ld_probe(vaddr, 0ULL);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    const bool fault_before_interrupts =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        fault_cause == CAUSE_LAF &&
        fault_tval == vaddr &&
        (CSRR(CSR_MIP) & (AI_IMC_SIP_SSIP | AI_IMC_MIP_STIP)) ==
            (AI_IMC_SIP_SSIP | AI_IMC_MIP_STIP) &&
        read64(paddr) == seed &&
        read64(guard) == guard_seed;

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, retry_payload);
    retry_read = ai_imc_ld_probe(vaddr, 0ULL);
    const bool retry_memory_once =
        !excpt.triggered &&
        retry_read == retry_payload &&
        read64(paddr) == retry_payload &&
        read64(guard) == guard_seed;
    ai_imc_clear_mprv_translation();

    TEST_SETUP_EXCEPT();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_SIE_SSIE | AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    const bool interrupts_delivered_after_repair =
        ai_imc_trap_count == 2ULL &&
        ai_imc_log_count_cause(CAUSE_SSI) == 1ULL &&
        ai_imc_log_count_cause(CAUSE_STI) == 1ULL &&
        (CSRR(CSR_MIP) & (AI_IMC_SIP_SSIP | AI_IMC_MIP_STIP)) == 0ULL &&
        read64(paddr) == retry_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP | AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("with SSIP and STIP both pending but disabled, the PMP load fault is reported first and preserves both pending sources",
        fault_before_interrupts);
    TEST_ASSERT("after PMP repair, the retry store/load completes once before pending interrupt delivery is enabled",
        retry_memory_once);
    TEST_ASSERT("after repair, both pending interrupts are delivered exactly once and do not change the repaired memory image",
        interrupts_delivered_after_repair,
        "trap_count=%lx cause0=%lx cause1=%lx target=%lx guard=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], read64(paddr), read64(guard));

    TEST_END(ai_interrupt_memory_m_ssip_stip_fault_retry_priority_corner);
}

bool ai_interrupt_memory_m_stip_load_use_dependency_result_corner()
{
    TEST_START();

    const uintptr_t load_addr = phys_page_base(SWITCH1) + 0x820U;
    const uintptr_t dependent_store = phys_page_base(SWITCH1) + 0x828U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x830U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x838U;
    const uint64_t load_seed = 0x696d636c64757001ULL;
    const uint64_t transform = 0x55aa33cc77881122ULL;
    const uint64_t marker_value = 0x696d636c64757002ULL;
    const uint64_t guard_seed = 0x696d636c6475aa03ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t loaded = 0ULL;
    uint64_t dependent_value = 0ULL;

    goto_priv(PRIV_M);
    write64(load_addr, load_seed);
    write64(dependent_store, 0ULL);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ld %[loaded], 0(%[load_addr])\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "xor %[dependent_value], %[loaded], %[transform]\n\t"
        "sd %[dependent_value], 0(%[store_addr])\n\t"
        ".option pop\n\t"
        : [loaded] "=&r"(loaded),
          [dependent_value] "=&r"(dependent_value)
        : [load_addr] "r"(load_addr),
          [store_addr] "r"(dependent_store),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE),
          [transform] "r"(transform)
        : "memory");
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const uint64_t expected_dependent = load_seed ^ transform;
    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool load_use_result_stable =
        loaded == load_seed &&
        dependent_value == expected_dependent &&
        read64(dependent_store) == expected_dependent &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP injected after a scalar load is logged exactly once before the dependent store window drains",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("the dependent store uses the mainline load value after interrupt return, not handler scratch or stale rd data",
        load_use_result_stable,
        "loaded=%lx dependent=%lx mem=%lx expected=%lx guard=%lx",
        loaded, dependent_value, read64(dependent_store),
        expected_dependent, read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_load_use_dependency_result_corner);
}

bool ai_interrupt_memory_m_stip_partial_store_forward_image_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH2) + 0x840U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x848U;
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t expected = 0xd4e5f607b2c3a188ULL;
    const uint64_t guard_seed = 0x696d63706677aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t observed = 0ULL;

    goto_priv(PRIV_M);
    write64(target, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sb %[byte_payload], 1(%[target])\n\t"
        "sh %[half_payload], 2(%[target])\n\t"
        "sw %[word_payload], 4(%[target])\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "ld %[observed], 0(%[target])\n\t"
        ".option pop\n\t"
        : [observed] "=&r"(observed)
        : [target] "r"(target),
          [byte_payload] "r"(0xa1ULL),
          [half_payload] "r"(0xb2c3ULL),
          [word_payload] "r"(0xd4e5f607ULL),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "memory");
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool partial_merge_stable =
        observed == expected &&
        read64(target) == expected &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP around a byte/half/word partial-store burst is delivered once and clears the source",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("the following full-width load and final memory image use the same merged byte mask/data owner",
        partial_merge_stable,
        "observed=%lx final=%lx expected=%lx guard=%lx",
        observed, read64(target), expected, read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_partial_store_forward_image_corner);
}

bool ai_interrupt_memory_m_stip_store_forward_raw_two_generation_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH1) + 0x1a00U;
    const uintptr_t handler_probe = phys_page_base(SWITCH2) + 0x1a00U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1a08U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1a10U;
    const uint64_t seed = 0x696d635257736565ULL;
    const uint64_t payload0 = 0x696d635257726177ULL;
    const uint64_t payload1 = 0x696d6352576e6577ULL;
    const uint64_t handler_seed = 0x696d63525768646cULL;
    const uint64_t marker0 = 0x696d6352576d6b30ULL;
    const uint64_t marker1 = 0x696d6352576d6b31ULL;
    const uint64_t guard_seed = 0x696d635257677561ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t load0 = 0ULL;
    uint64_t load1 = 0ULL;
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;

    goto_priv(PRIV_M);
    write64(target, seed);
    write64(handler_probe, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[payload], 0(%[target])\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "ld %[load], 0(%[target])\n\t"
        ".option pop\n\t"
        : [load] "=&r"(load0)
        : [target] "r"(target),
          [payload] "r"(payload0),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "memory");
    rounds0 = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker0);
    fence_iorw_iorw();

    const bool first_generation_forwarded =
        rounds0 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        load0 == payload0 &&
        read64(target) == payload0 &&
        read64(marker) == marker0 &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sd %[payload], 0(%[target])\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "ld %[load], 0(%[target])\n\t"
        ".option pop\n\t"
        : [load] "=&r"(load1)
        : [target] "r"(target),
          [payload] "r"(payload1),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "memory");
    rounds1 = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker1);
    fence_iorw_iorw();

    const bool second_generation_rechecked =
        rounds1 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[1] == handler_seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        load1 == payload1 &&
        read64(target) == payload1 &&
        read64(marker) == marker1 &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool no_stale_or_handler_data =
        load0 != seed &&
        load1 != seed &&
        load1 != payload0 &&
        ai_imc_handler_probe_value[0] != payload0 &&
        ai_imc_handler_probe_value[1] != payload1 &&
        read64(handler_probe) == handler_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("first same-address store/load generation around STIP observes the older store payload and logs one handler probe",
        first_generation_forwarded,
        "rounds=%lx trap_count=%lx cause0=%lx load0=%lx target=%lx marker=%lx probe0=%lx final_mip=%lx",
        rounds0, ai_imc_trap_count, ai_imc_trap_cause[0],
        load0, read64(target), read64(marker),
        ai_imc_handler_probe_value[0], CSRR(CSR_MIP));
    TEST_ASSERT("second same-address generation is rechecked after another STIP and does not reuse the old payload",
        second_generation_rechecked,
        "rounds=%lx trap_count=%lx cause1=%lx load1=%lx target=%lx marker=%lx logs=%lx,%lx final_mip=%lx",
        rounds1, ai_imc_trap_count, ai_imc_trap_cause[1],
        load1, read64(target), read64(marker),
        ai_imc_handler_log_page[0], ai_imc_handler_log_page[1],
        CSRR(CSR_MIP));
    TEST_ASSERT("RAW smoke keeps stale seed data and handler probe data out of both mainline load results",
        no_stale_or_handler_data,
        "seed=%lx payload0=%lx payload1=%lx load0=%lx load1=%lx probe=%lx guard=%lx",
        seed, payload0, payload1, load0, load1, read64(handler_probe),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_store_forward_raw_two_generation_corner);
}

bool ai_interrupt_memory_m_stip_partial_overlap_negative_forward_smoke_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH2) + 0x860U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x868U;
    const uint64_t seed = 0x1122334455667788ULL;
    const uint64_t expected_full = 0xa1b2334455667788ULL;
    const uint64_t expected_lwu = 0x00000000a1b23344ULL;
    const uint64_t guard_seed = 0x696d636e6667aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t observed_lwu = 0ULL;
    uint64_t observed_full = 0ULL;

    goto_priv(PRIV_M);
    write64(target, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "sh %[half_payload], 6(%[target])\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        "lwu %[observed_lwu], 4(%[target])\n\t"
        "ld %[observed_full], 0(%[target])\n\t"
        ".option pop\n\t"
        : [observed_lwu] "=&r"(observed_lwu),
          [observed_full] "=&r"(observed_full)
        : [target] "r"(target),
          [half_payload] "r"(0xa1b2ULL),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "memory");
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool partial_overlap_image =
        !excpt.triggered &&
        observed_lwu == expected_lwu &&
        observed_full == expected_full &&
        read64(target) == expected_full &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP adjacent to a partial-overlap store/load pair is delivered once and clears before exit",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("the overlapping load observes only the two updated bytes plus preserved memory bytes, not an illegal full-store forward payload",
        partial_overlap_image,
        "lwu=%lx full=%lx final=%lx expected_lwu=%lx expected_full=%lx guard=%lx excpt=%d",
        observed_lwu, observed_full, read64(target), expected_lwu,
        expected_full, read64(guard), excpt.triggered);

    TEST_END(ai_interrupt_memory_m_stip_partial_overlap_negative_forward_smoke_corner);
}

bool ai_interrupt_memory_m_stip_store_burst_post_handler_progress_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SCRATCHPAD) + 0x880U;
    const uintptr_t post_handler = phys_page_base(SCRATCHPAD) + 0x8e0U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x8e8U;
    const uint64_t payload_base = 0x696d637362757200ULL;
    const uint64_t seed_base = 0x696d637362737300ULL;
    const uint64_t post_value = 0x696d637362706f73ULL;
    const uint64_t guard_seed = 0x696d63736275aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    bool burst_image_ok = true;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 12ULL; ++i) {
        write64(base + i * sizeof(uint64_t), seed_base + i);
    }
    write64(post_handler, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(base + i * sizeof(uint64_t), payload_base + i);
    }
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(post_handler, post_value);
    for (uint64_t i = 8ULL; i < 12ULL; ++i) {
        write64(base + i * sizeof(uint64_t), payload_base + i);
    }
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 12ULL; ++i) {
        if (read64(base + i * sizeof(uint64_t)) != payload_base + i) {
            burst_image_ok = false;
        }
    }

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool post_handler_store_progress =
        read64(post_handler) == post_value &&
        read64(guard) == guard_seed &&
        burst_image_ok;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("near-burst STIP is delivered once between the pre-burst stores and first post-handler sentinel store",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("all burst stores plus the first post-handler sentinel store remain visible exactly once",
        post_handler_store_progress,
        "post=%lx guard=%lx first=%lx last=%lx",
        read64(post_handler), read64(guard),
        read64(base), read64(base + 11ULL * sizeof(uint64_t)));

    TEST_END(ai_interrupt_memory_m_stip_store_burst_post_handler_progress_corner);
}

bool ai_interrupt_memory_m_stip_handler_spill_stack_isolation_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH1) + 0x1800U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1880U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1888U;
    const uint64_t seed_base = 0x696d637373706500ULL;
    const uint64_t payload_base = 0x696d637373706c00ULL;
    const uint64_t marker_value = 0x696d637373706d6bULL;
    const uint64_t guard_seed = 0x696d637373706775ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    bool pre_burst_ok = true;
    bool final_burst_ok = true;
    uint64_t pre_sum = 0ULL;
    uint64_t final_sum = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 16ULL; ++i) {
        write64(target + i * sizeof(uint64_t), seed_base + i);
    }
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    ai_imc_reset_trap_log();
    ai_imc_handler_spill_guard0 = 0x696d637373706730ULL;
    ai_imc_handler_spill_guard1 = 0x696d637373706731ULL;
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(target + i * sizeof(uint64_t), payload_base + i);
    }
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        pre_sum += read64(target + i * sizeof(uint64_t));
        if (read64(target + i * sizeof(uint64_t)) != payload_base + i) {
            pre_burst_ok = false;
        }
    }

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_spill_stack_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    for (uint64_t i = 8ULL; i < 16ULL; ++i) {
        write64(target + i * sizeof(uint64_t), payload_base + i);
    }
    write64(marker, marker_value);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 16ULL; ++i) {
        const uint64_t observed = read64(target + i * sizeof(uint64_t));
        final_sum += observed;
        if (observed != payload_base + i) {
            final_burst_ok = false;
        }
    }

    const bool interrupt_spilled_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_saved_sp[0] != 0ULL &&
        ai_imc_handler_spill_stack[0] == 0x696d6373706c0000ULL &&
        ai_imc_handler_spill_stack[1] == 0x696d6373706c0011ULL &&
        ai_imc_handler_spill_stack[2] == 0x696d6373706c0022ULL &&
        ai_imc_handler_spill_stack[3] == 0x696d6373706c0033ULL &&
        ai_imc_handler_spill_guard0 == 0x696d637373706730ULL &&
        ai_imc_handler_spill_guard1 == 0x696d637373706731ULL;
    const bool target_stack_isolated =
        pre_burst_ok &&
        final_burst_ok &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        target != (uintptr_t)&ai_imc_handler_spill_stack[0] &&
        marker != (uintptr_t)&ai_imc_handler_spill_stack[0] &&
        guard != (uintptr_t)&ai_imc_handler_spill_stack[0] &&
        final_sum != pre_sum;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("custom STIP handler spills/fills on its own stack once and clears the interrupt source",
        interrupt_spilled_once,
        "rounds=%lx trap_count=%lx cause=%lx saved_sp=%lx spill=%lx,%lx,%lx,%lx guards=%lx,%lx mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_saved_sp[0], ai_imc_handler_spill_stack[0],
        ai_imc_handler_spill_stack[1], ai_imc_handler_spill_stack[2],
        ai_imc_handler_spill_stack[3], ai_imc_handler_spill_guard0,
        ai_imc_handler_spill_guard1, CSRR(CSR_MIP));
    TEST_ASSERT("mainline memory-pressure target, marker, and guard stay disjoint from the handler spill stack",
        target_stack_isolated,
        "pre_sum=%lx final_sum=%lx target0=%lx target15=%lx marker=%lx guard=%lx",
        pre_sum, final_sum, read64(target),
        read64(target + 15ULL * sizeof(uint64_t)), read64(marker),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_handler_spill_stack_isolation_corner);
}

bool ai_interrupt_memory_m_stip_interrupt_storm_memory_progress_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH1) + 0x900U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x940U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x960U;
    const uint64_t payload_base = 0x696d6373746f7200ULL;
    const uint64_t marker_xor = 0x0101010101010101ULL;
    const uint64_t guard_seed = 0x696d6373746faa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    bool memory_progress_ok = true;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        write64(base + i * sizeof(uint64_t), 0ULL);
        write64(marker + i * sizeof(uint64_t), 0ULL);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        const uint64_t payload = payload_base + i;
        write64(base + i * sizeof(uint64_t), payload);
        const uint64_t loaded = read64(base + i * sizeof(uint64_t));
        CSRS(CSR_MIP, AI_IMC_MIP_STIP);
        (void)ai_imc_spin_until_log_count(i + 1ULL, AI_IMC_SPIN_LIMIT);
        write64(marker + i * sizeof(uint64_t), loaded ^ marker_xor);
    }
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < AI_IMC_LOG_SLOTS; ++i) {
        const uint64_t payload = payload_base + i;
        if (ai_imc_trap_cause[i] != CAUSE_STI ||
            read64(base + i * sizeof(uint64_t)) != payload ||
            read64(marker + i * sizeof(uint64_t)) !=
                (payload ^ marker_xor)) {
            memory_progress_ok = false;
        }
    }

    const bool interrupt_storm_logged =
        ai_imc_trap_count == AI_IMC_LOG_SLOTS &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool guard_stable = read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("four CSR STIP injections are delivered and cleared without ghost reentry during the memory burst loop",
        interrupt_storm_logged,
        "trap_count=%lx causes=%lx/%lx/%lx/%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_cause[2],
        ai_imc_trap_cause[3], CSRR(CSR_MIP));
    TEST_ASSERT("each storm round preserves store-load-marker progress and the independent guard image",
        memory_progress_ok && guard_stable,
        "base0=%lx marker0=%lx base3=%lx marker3=%lx guard=%lx",
        read64(base), read64(marker),
        read64(base + 3ULL * sizeof(uint64_t)),
        read64(marker + 3ULL * sizeof(uint64_t)), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_interrupt_storm_memory_progress_corner);
}

bool ai_interrupt_memory_m_stip_sc_failure_storm_status_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0x980U;
    const uintptr_t alias = phys_page_base(SCRATCHPAD) + 0x988U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0x990U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x998U;
    const uint64_t seed = 0x696d637363660001ULL;
    const uint64_t alias_payload0 = 0x696d637363666110ULL;
    const uint64_t alias_payload1 = 0x696d637363666111ULL;
    const uint64_t success_payload = 0x696d637363667301ULL;
    const uint64_t marker_value = 0x696d637363666d01ULL;
    const uint64_t guard_seed = 0x696d63736366aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t lr0 = 0ULL;
    uint64_t lr1 = 0ULL;
    uint64_t lr_success = 0ULL;
    uint64_t sc_fail0 = 0ULL;
    uint64_t sc_fail1 = 0ULL;
    uint64_t sc_success = 1ULL;

    goto_priv(PRIV_M);
    write64(target, seed);
    write64(alias, 0ULL);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    lr0 = ai_imc_lr_d((uint64_t *)target);
    write64(alias, alias_payload0);
    sc_fail0 = ai_imc_sc_d((uint64_t *)alias, alias_payload0 + 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    lr1 = ai_imc_lr_d((uint64_t *)target);
    write64(alias, alias_payload1);
    sc_fail1 = ai_imc_sc_d((uint64_t *)alias, alias_payload1 + 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);

    lr_success = ai_imc_lr_d((uint64_t *)target);
    sc_success = ai_imc_sc_d((uint64_t *)target, success_payload);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool failure_statuses_are_fresh =
        lr0 == seed &&
        lr1 == seed &&
        sc_fail0 != 0ULL &&
        sc_fail1 != 0ULL &&
        read64(target) == success_payload &&
        read64(alias) == alias_payload1;
    const bool success_round_is_exact =
        lr_success == seed &&
        sc_success == 0ULL &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;
    const bool storm_logged =
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("two SC failure rounds around STIP storm report fresh nonzero status and do not write the LR target",
        failure_statuses_are_fresh,
        "lr0=%lx lr1=%lx sc0=%lx sc1=%lx target=%lx alias=%lx",
        lr0, lr1, sc_fail0, sc_fail1, read64(target), read64(alias));
    TEST_ASSERT("the final fresh LR/SC success updates the target once after the failure storm",
        success_round_is_exact,
        "lr_success=%lx sc_success=%lx target=%lx marker=%lx guard=%lx",
        lr_success, sc_success, read64(target), read64(marker),
        read64(guard));
    TEST_ASSERT("both storm STIP injections are logged exactly once and cleared before case exit",
        storm_logged,
        "trap_count=%lx cause0=%lx cause1=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], CSRR(CSR_MIP));

    TEST_END(ai_interrupt_memory_m_stip_sc_failure_storm_status_corner);
}

bool ai_interrupt_memory_m_stip_fp_int_store_dirty_owner_corner()
{
    TEST_START();

    const uintptr_t fp_load = phys_page_base(SWITCH2) + 0x9c0U;
    const uintptr_t fp_store = phys_page_base(SWITCH2) + 0x9c8U;
    const uintptr_t int_store = phys_page_base(SWITCH2) + 0x9d0U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x9d8U;
    const uint64_t fp_load_bits = 0x7ff8123456789abcULL;
    const uint64_t fp_store_bits = 0x400921fb54442d18ULL;
    const uint64_t int_store_bits = 0x696d636670696e74ULL;
    const uint64_t fp_store_seed = 0x696d636670737365ULL;
    const uint64_t int_store_seed = 0x696d636970737365ULL;
    const uint64_t guard_seed = 0x696d63667069aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t mstatus_clean = 0ULL;
    uint64_t loaded_bits = 0ULL;
    uint64_t fs_after_load = 0ULL;
    uint64_t fs_after_stores = 0ULL;

    goto_priv(PRIV_M);
    write64(fp_load, fp_load_bits);
    write64(fp_store, fp_store_seed);
    write64(int_store, int_store_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    mstatus_clean = CSRR(CSR_MSTATUS);
    mstatus_clean &= ~SSTATUS_FS_MSK;
    mstatus_clean |= SSTATUS_FS_CLEAN;
    CSRW(CSR_MSTATUS, mstatus_clean);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    loaded_bits = ai_imc_fld_raw_bits(fp_load);
    fs_after_load = CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    ai_imc_fsd_raw_bits(fp_store, fp_store_bits);
    write64(int_store, int_store_bits);
    fence_iorw_iorw();
    fs_after_stores = CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK;

    const bool fp_result_and_dirty =
        loaded_bits == fp_load_bits &&
        fs_after_load == SSTATUS_FS_DIRTY &&
        fs_after_stores == SSTATUS_FS_DIRTY;
    const bool store_payloads_separate =
        read64(fp_store) == fp_store_bits &&
        read64(int_store) == int_store_bits &&
        read64(guard) == guard_seed;
    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("FP load around STIP returns the FP memory payload and sets FS dirty from the committed FP op",
        fp_result_and_dirty,
        "loaded=%lx fs_load=%lx fs_store=%lx",
        loaded_bits, fs_after_load, fs_after_stores);
    TEST_ASSERT("FP store payload and integer store payload remain separate after interrupt return",
        store_payloads_separate,
        "fp_store=%lx int_store=%lx guard=%lx",
        read64(fp_store), read64(int_store), read64(guard));
    TEST_ASSERT("STIP adjacent to FP/integer memory stores is logged once and does not alter target memory",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);

    TEST_END(ai_interrupt_memory_m_stip_fp_int_store_dirty_owner_corner);
}

bool ai_interrupt_memory_hs_stip_fp_load_fault_repair_handler_int_owner_corner()
{
    TEST_START();

    const enum test_page low_page = VSR_GX;
    const enum test_page high_page = VSW_GI;
    const enum test_page handler_page = SCRATCHPAD;
    const uintptr_t load_vaddr = hs_page_base(low_page) + PAGE_SIZE - 4U;
    const uintptr_t high_vaddr = hs_page_base(high_page);
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t high_paddr = phys_page_base(high_page);
    const uintptr_t handler_probe = phys_page_base(handler_page) + 0xae0U;
    const uintptr_t guard = phys_page_base(handler_page) + 0xae8U;
    const uint64_t low_pattern = 0x8798a9bacbdcedfeULL;
    const uint64_t high_pattern = 0x1032547698badcfeULL;
    const uint64_t handler_probe_seed = 0x696d63667068696eULL;
    const uint64_t guard_seed = 0x696d63667068aa01ULL;
    const uint64_t fpr_sentinel = 0x7ff80000deadbabeULL;
    const uint64_t expected_fld_bits =
        ((high_pattern & 0xffffffffULL) << 32U) |
        (low_pattern >> 32U);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t fs_after_fault = 0ULL;
    uint64_t fpr_after_fault = 0ULL;
    bool fault_triggered = false;
    uint64_t interrupt_rounds = 0ULL;
    uint64_t repaired_bits = 0ULL;
    uint64_t integer_twin = 0ULL;
    uint64_t fs_after_repair = 0ULL;
    bool fp_retry_triggered = false;
    bool integer_twin_triggered = false;

    goto_priv(PRIV_M);
    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    write64(low_paddr, low_pattern);
    write64(high_paddr, high_pattern);
    write64(handler_probe, handler_probe_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(handler_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    asm volatile("fmv.d.x ft0, %0" : : "r"(fpr_sentinel) : "ft0");
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_CLEAN);
    TEST_SETUP_EXCEPT();
    ai_imc_fld_only(load_vaddr);
    fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fs_after_fault = CSRR(CSR_SSTATUS) & SSTATUS_FS_MSK;
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    asm volatile("fmv.x.d %0, ft0" : "=r"(fpr_after_fault));

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool fp_fault_contract =
        fault_triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == high_vaddr &&
        fs_after_fault == SSTATUS_FS_CLEAN &&
        fpr_after_fault == fpr_sentinel &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern;

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | SSTATUS_FS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool handler_integer_load_isolated =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_probe_value[0] == handler_probe_seed &&
        ai_imc_handler_probe_value[0] != fpr_sentinel &&
        ai_imc_handler_probe_value[0] != expected_fld_bits &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(handler_probe) == handler_probe_seed &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    CSRS(CSR_SSTATUS, SSTATUS_FS_DIRTY);
    asm volatile("fmv.d.x ft0, %0" : : "r"(fpr_sentinel) : "ft0");
    CSRC(CSR_SSTATUS, SSTATUS_FS_MSK);
    CSRS(CSR_SSTATUS, SSTATUS_FS_CLEAN);
    TEST_SETUP_EXCEPT();
    repaired_bits = ai_imc_fld_sentinel_probe(load_vaddr, fpr_sentinel);
    fp_retry_triggered = excpt.triggered;
    fs_after_repair = CSRR(CSR_SSTATUS) & SSTATUS_FS_MSK;
    TEST_SETUP_EXCEPT();
    integer_twin = ld(load_vaddr);
    integer_twin_triggered = excpt.triggered;
    const bool repaired_fp_and_integer_owner =
        !fp_retry_triggered &&
        !integer_twin_triggered &&
        repaired_bits == expected_fld_bits &&
        integer_twin == expected_fld_bits &&
        fs_after_repair == SSTATUS_FS_DIRTY;

    goto_priv(PRIV_M);
    const bool backing_image_stable =
        read64(low_paddr) == low_pattern &&
        read64(high_paddr) == high_pattern &&
        read64(handler_probe) == handler_probe_seed &&
        read64(guard) == guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("cross-page Clean-FS fld reports the high-page fault while preserving FPR sentinel, FS Clean, backing data, and pending STIP",
        fp_fault_contract,
        "cause=%lx tval=%lx expected=%lx fs=%lx fpr=%lx pending=%lx low=%lx high=%lx",
        fault_cause, fault_tval, high_vaddr, fs_after_fault,
        fpr_after_fault, pending_after_fault, read64(low_paddr),
        read64(high_paddr));
    TEST_ASSERT("pending STIP handler performs exactly one integer load from its own probe target and keeps that payload out of the FP oracle",
        handler_integer_load_isolated,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx expected_probe=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], handler_probe_seed,
        CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the high leaf, the retry fld and integer twin read the same merged payload and dirty FS only on commit",
        repaired_fp_and_integer_owner && backing_image_stable,
        "fp=%lx integer=%lx expected=%lx fs=%lx low=%lx high=%lx handler=%lx guard=%lx",
        repaired_bits, integer_twin, expected_fld_bits, fs_after_repair,
        read64(low_paddr), read64(high_paddr), read64(handler_probe),
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_fp_load_fault_repair_handler_int_owner_corner);
}

bool ai_interrupt_memory_m_stip_vector_vs_dirty_owner_corner()
{
    TEST_START();

    const uintptr_t src0 = phys_page_base(SWITCH1) + 0x1d00U;
    const uintptr_t dst0 = phys_page_base(SWITCH1) + 0x1d40U;
    const uintptr_t src1 = phys_page_base(SWITCH1) + 0x1d80U;
    const uintptr_t dst1 = phys_page_base(SWITCH1) + 0x1dc0U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1e00U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1e08U;
    const uint32_t src0_words[AI_IMC_VEC_VL4] = {
        0x76010001U, 0x76010002U, 0x76010003U, 0x76010004U
    };
    const uint32_t src1_words[AI_IMC_VEC_VL4] = {
        0x76020011U, 0x76020012U, 0x76020013U, 0x76020014U
    };
    const uint64_t marker_value = 0x696d6376736f776eULL;
    const uint64_t guard_seed = 0x696d637676736401ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t mstatus_clean = 0ULL;
    uint64_t vs_after_pre_interrupt = 0ULL;
    uint64_t vs_after_first_vector = 0ULL;
    uint64_t vs_after_second_vector = 0ULL;
    uint64_t vs_after_post_interrupt = 0ULL;
    uint64_t first_vstart = 0xffffffffffffffffULL;
    uint64_t second_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(src0 + i * sizeof(uint32_t), src0_words[i]);
        write32(src1 + i * sizeof(uint32_t), src1_words[i]);
        write32(dst0 + i * sizeof(uint32_t), 0xeeee0000U + (uint32_t)i);
        write32(dst1 + i * sizeof(uint32_t), 0xdddd0000U + (uint32_t)i);
    }
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_vec_prepare_e32m1_vl4();
    mstatus_clean = CSRR(CSR_MSTATUS);
    mstatus_clean &= ~AI_IMC_MSTATUS_VS_MASK;
    mstatus_clean |= AI_IMC_MSTATUS_VS_CLEAN;
    CSRW(CSR_MSTATUS, mstatus_clean);

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    vs_after_pre_interrupt = CSRR(CSR_MSTATUS) & AI_IMC_MSTATUS_VS_MASK;

    TEST_SETUP_EXCEPT();
    ai_imc_vec_copy_current_vl_u32((const uint32_t *)src0,
        (uint32_t *)dst0, &first_vstart);
    const bool first_vector_no_fault = !excpt.triggered;
    vs_after_first_vector = CSRR(CSR_MSTATUS) & AI_IMC_MSTATUS_VS_MASK;

    ai_imc_vec_prepare_e32m1_vl4();
    CSRW(CSR_MSTATUS, mstatus_clean);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_copy_current_vl_u32((const uint32_t *)src1,
        (uint32_t *)dst1, &second_vstart);
    const bool second_vector_no_fault = !excpt.triggered;
    vs_after_second_vector = CSRR(CSR_MSTATUS) & AI_IMC_MSTATUS_VS_MASK;
    write64(marker, marker_value);
    fence_iorw_iorw();

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    vs_after_post_interrupt = CSRR(CSR_MSTATUS) & AI_IMC_MSTATUS_VS_MASK;

    const bool interrupt_logged_twice =
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool vs_owner_contract =
        vs_after_pre_interrupt == AI_IMC_MSTATUS_VS_CLEAN &&
        first_vector_no_fault &&
        first_vstart == 0ULL &&
        vs_after_first_vector == AI_IMC_MSTATUS_VS_DIRTY &&
        second_vector_no_fault &&
        second_vstart == 0ULL &&
        vs_after_second_vector == AI_IMC_MSTATUS_VS_DIRTY &&
        vs_after_post_interrupt == AI_IMC_MSTATUS_VS_DIRTY;
    const bool vector_memory_owner_ok =
        ai_imc_words_match(dst0, src0_words, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(dst1, src1_words, AI_IMC_VEC_VL4) &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP before and after vector memory is logged exactly once per injection and clears source state",
        interrupt_logged_twice,
        "trap_count=%lx cause0=%lx cause1=%lx mip0=%lx mip1=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_mip[0],
        ai_imc_trap_mip[1], CSRR(CSR_MIP));
    TEST_ASSERT("VS remains clean across the pre-vector interrupt and becomes dirty only after committed vector memory",
        vs_owner_contract,
        "vs_pre_int=%lx vs_vec0=%lx vs_vec1=%lx vs_post_int=%lx vstart0=%lx vstart1=%lx nofault0=%d nofault1=%d",
        vs_after_pre_interrupt, vs_after_first_vector,
        vs_after_second_vector, vs_after_post_interrupt,
        first_vstart, second_vstart, first_vector_no_fault,
        second_vector_no_fault);
    TEST_ASSERT("vector store images remain bound to their own source payloads and handler marker does not overwrite them",
        vector_memory_owner_ok,
        "dst0=%x,%x,%x,%x dst1=%x,%x,%x,%x marker=%lx guard=%lx",
        read32(dst0), read32(dst0 + 4U), read32(dst0 + 8U),
        read32(dst0 + 12U), read32(dst1), read32(dst1 + 4U),
        read32(dst1 + 8U), read32(dst1 + 12U), read64(marker),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_vs_dirty_owner_corner);
}

bool ai_interrupt_memory_m_stip_load_queue_pressure_younger_load_corner()
{
    TEST_START();

    const uintptr_t older_base = phys_page_base(SWITCH1) + 0xa00U;
    const uintptr_t younger_base = phys_page_base(SWITCH1) + 0xa80U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0xb00U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xb08U;
    const uint64_t older_seed = 0x696d636c716f0000ULL;
    const uint64_t younger_seed = 0x696d636c71790000ULL;
    const uint64_t guard_seed = 0x696d636c71aa0001ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t older_sum = 0ULL;
    uint64_t older_xor = 0ULL;
    uint64_t younger_sum = 0ULL;
    uint64_t younger_xor = 0ULL;
    uint64_t expected_older_sum = 0ULL;
    uint64_t expected_older_xor = 0ULL;
    uint64_t expected_younger_sum = 0ULL;
    uint64_t expected_younger_xor = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        const uint64_t older_payload = older_seed + i;
        const uint64_t younger_payload = younger_seed + (i << 4U);

        write64(older_base + i * sizeof(uint64_t), older_payload);
        write64(younger_base + i * sizeof(uint64_t), younger_payload);
        expected_older_sum += older_payload;
        expected_older_xor ^= older_payload;
        expected_younger_sum += younger_payload;
        expected_younger_xor ^= younger_payload;
    }
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_load8_signature(older_base, &older_sum, &older_xor);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    ai_imc_load8_signature(younger_base, &younger_sum, &younger_xor);
    write64(marker, younger_sum ^ younger_xor);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool older_signature_stable =
        older_sum == expected_older_sum &&
        older_xor == expected_older_xor;
    const bool younger_signature_not_stale =
        younger_sum == expected_younger_sum &&
        younger_xor == expected_younger_xor &&
        younger_sum != older_sum &&
        read64(marker) == (expected_younger_sum ^ expected_younger_xor) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between an older eight-load burst and a younger eight-load burst is logged exactly once and cleared",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("the older load burst keeps its own payload signature before the interrupt",
        older_signature_stable,
        "older_sum=%lx expected=%lx older_xor=%lx expected_xor=%lx",
        older_sum, expected_older_sum, older_xor, expected_older_xor);
    TEST_ASSERT("the younger post-handler load burst uses the younger payloads and not stale older load metadata",
        younger_signature_not_stale,
        "younger_sum=%lx expected=%lx younger_xor=%lx expected_xor=%lx marker=%lx guard=%lx",
        younger_sum, expected_younger_sum, younger_xor,
        expected_younger_xor, read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_load_queue_pressure_younger_load_corner);
}

bool ai_interrupt_memory_m_stip_rvc_load_store_pc2_mask_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SWITCH2) + 0xa00U;
    const uintptr_t sd_target = phys_page_base(SWITCH2) + 0xa08U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0xa10U;
    const uintptr_t marker = phys_page_base(SWITCH2) + 0xa18U;
    const uint64_t initial = 0x1122334455667788ULL;
    const uint32_t sw_payload = 0xa1b2c3d4U;
    const uint64_t sd_payload = 0x8877665544332211ULL;
    const uint64_t expected_target = 0x11223344a1b2c3d4ULL;
    const uint64_t guard_seed = 0x696d63727663aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    ai_imc_rvc_probe_result_t probe = {0};

    goto_priv(PRIV_M);
    write64(target, initial);
    write64(sd_target, 0ULL);
    write64(guard, guard_seed);
    write64(marker, 0ULL);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_rvc_memory_probe(target, sd_target, sd_payload, sw_payload,
        &probe);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, probe.loaded ^ read64(target) ^ read64(sd_target));
    fence_iorw_iorw();

    const bool compressed_pc2_layout =
        probe.cld_next == probe.cld_pc + 2U &&
        probe.csw_next == probe.csw_pc + 2U &&
        probe.csd_next == probe.csd_pc + 2U;
    const bool compressed_memory_image =
        probe.loaded == initial &&
        read64(target) == expected_target &&
        read64(sd_target) == sd_payload &&
        read64(guard) == guard_seed &&
        read64(marker) == (initial ^ expected_target ^ sd_payload);
    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the explicit C.LD/C.SW/C.SD memory sequence is laid out as three 16-bit instructions",
        compressed_pc2_layout,
        "cld=%lx/%lx csw=%lx/%lx csd=%lx/%lx",
        probe.cld_pc, probe.cld_next, probe.csw_pc, probe.csw_next,
        probe.csd_pc, probe.csd_next);
    TEST_ASSERT("RVC load/store results around STIP keep the C.SW word mask, C.SD payload and marker exactly once",
        compressed_memory_image,
        "loaded=%lx target=%lx sd=%lx marker=%lx guard=%lx",
        probe.loaded, read64(target), read64(sd_target), read64(marker),
        read64(guard));
    TEST_ASSERT("STIP adjacent to the compressed memory sequence is delivered once and does not repeat after return",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);

    TEST_END(ai_interrupt_memory_m_stip_rvc_load_store_pc2_mask_corner);
}

bool ai_interrupt_memory_m_stip_rvc_fp_load_store_pc2_dirty_corner()
{
    TEST_START();

    const uintptr_t load_base = phys_page_base(SWITCH2) + 0xa40U;
    const uintptr_t store_base = phys_page_base(SWITCH2) + 0xa80U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0xac0U;
    const uint64_t load_guard0 = 0x696d636366706700ULL;
    const uint64_t load_bits = 0x7ff8123456789abcULL;
    const uint64_t load_guard1 = 0x696d636366706701ULL;
    const uint64_t store_guard0 = 0x696d636366707300ULL;
    const uint64_t store_seed = 0x696d636366707301ULL;
    const uint64_t store_bits = 0x400921fb54442d18ULL;
    const uint64_t store_guard1 = 0x696d636366707302ULL;
    const uint64_t guard_seed = 0x696d63636670aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    ai_imc_rvc_fp_probe_result_t probe = {0};
    uint64_t mstatus_clean = 0ULL;
    uint64_t fs_after_probe = 0ULL;

    goto_priv(PRIV_M);
    write64(load_base + 0U, load_guard0);
    write64(load_base + 16U, load_bits);
    write64(load_base + 24U, load_guard1);
    write64(store_base + 16U, store_guard0);
    write64(store_base + 24U, store_seed);
    write64(store_base + 32U, store_guard1);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    CSRS(CSR_MSTATUS, SSTATUS_FS_DIRTY);
    ai_imc_seed_fs1_raw_bits(store_bits);
    mstatus_clean = CSRR(CSR_MSTATUS);
    mstatus_clean &= ~SSTATUS_FS_MSK;
    mstatus_clean |= SSTATUS_FS_CLEAN;
    CSRW(CSR_MSTATUS, mstatus_clean);

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_rvc_fp_load_store_probe(load_base, store_base, &probe);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fs_after_probe = CSRR(CSR_MSTATUS) & SSTATUS_FS_MSK;
    fence_iorw_iorw();

    const bool compressed_fp_pc2_layout =
        probe.cfld_next == probe.cfld_pc + 2U &&
        probe.cfsd_next == probe.cfsd_pc + 2U;
    const bool fp_memory_image =
        !excpt.triggered &&
        probe.loaded == load_bits &&
        read64(load_base + 0U) == load_guard0 &&
        read64(load_base + 16U) == load_bits &&
        read64(load_base + 24U) == load_guard1 &&
        read64(store_base + 16U) == store_guard0 &&
        read64(store_base + 24U) == store_bits &&
        read64(store_base + 32U) == store_guard1 &&
        read64(guard) == guard_seed;
    const bool fp_dirty_and_interrupt =
        fs_after_probe == SSTATUS_FS_DIRTY &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("explicit C.FLD/C.FSD instructions around STIP have 16-bit PC spans",
        compressed_fp_pc2_layout,
        "cfld=%lx/%lx cfsd=%lx/%lx",
        probe.cfld_pc, probe.cfld_next, probe.cfsd_pc,
        probe.cfsd_next);
    TEST_ASSERT("compressed FP load/store keep raw double payload, 8-byte store width and adjacent guards",
        fp_memory_image,
        "loaded=%lx load=%lx store=%lx guards=%lx,%lx,%lx",
        probe.loaded, read64(load_base + 16U),
        read64(store_base + 24U), read64(load_base),
        read64(store_base + 16U), read64(guard));
    TEST_ASSERT("STIP adjacent to compressed FP memory is delivered once and FP Dirty belongs to the committed FP ops",
        fp_dirty_and_interrupt,
        "fs=%lx trap_count=%lx cause=%lx mip=%lx",
        fs_after_probe, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);

    TEST_END(ai_interrupt_memory_m_stip_rvc_fp_load_store_pc2_dirty_corner);
}

bool ai_interrupt_memory_hs_stip_compressed_store_fault_pc2_retry_corner()
{
    TEST_START();

    const enum test_page store_page = VSRWX_GRW;
    const enum test_page marker_page = VSRW_GRWX;
    const uintptr_t store_paddr = phys_page_base(store_page) + 0x180U;
    const uintptr_t store_vaddr = hs_page_base(store_page) + 0x180U;
    const uintptr_t guard_before = store_paddr - sizeof(uint64_t);
    const uintptr_t guard_after = store_paddr + sizeof(uint64_t);
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0x180U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0x180U;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t no_write_leaf = PTE_V | PTE_AD | PTE_R;
    const uint64_t store_seed = 0x696d637663736430ULL;
    const uint64_t guard_before_seed = 0x696d637663676201ULL;
    const uint64_t guard_after_seed = 0x696d637663676102ULL;
    const uint64_t marker_initial = 0x696d637276636900ULL;
    const uint64_t fault_marker = 0x696d637276636601ULL;
    const uint64_t retry_marker = 0x696d637276637202ULL;
    const uint64_t fault_payload = 0xfedcba9876543210ULL;
    const uint64_t retry_payload = 0x8877665544332211ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    ai_imc_rvc_store_fault_pc_t fault_pc = {0};
    ai_imc_rvc_store_fault_pc_t retry_pc = {0};
    uint64_t fault_cause = 0ULL;
    uint64_t fault_epc = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;
    bool retry_no_fault = false;

    goto_priv(PRIV_M);
    write64(guard_before, guard_before_seed);
    write64(store_paddr, store_seed);
    write64(guard_after, guard_after_seed);
    write64(marker_paddr, marker_initial);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(store_page, no_write_leaf);
    ai_imc_set_hs_leaf(marker_page, legal_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_imc_rvc_csd_marker_probe(store_vaddr, fault_payload, marker_vaddr,
        fault_marker, &fault_pc);
    fault_cause = excpt.cause;
    fault_epc = excpt.epc;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool compressed_fault_metadata_ok =
        excpt.triggered &&
        fault_cause == CAUSE_SPF &&
        fault_epc == fault_pc.pc &&
        fault_tval == store_vaddr &&
        fault_pc.next_pc == fault_pc.pc + 2U &&
        read64(marker_paddr) == fault_marker &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;
    const bool compressed_fault_no_side_effect =
        read64(guard_before) == guard_before_seed &&
        read64(store_paddr) == store_seed &&
        read64(guard_after) == guard_after_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool pending_stip_delivered_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL;

    write64(guard_before, guard_before_seed);
    write64(store_paddr, store_seed);
    write64(guard_after, guard_after_seed);
    write64(marker_paddr, marker_initial);
    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(store_page, legal_leaf);
    TEST_SETUP_EXCEPT();
    ai_imc_rvc_csd_marker_probe(store_vaddr, retry_payload, marker_vaddr,
        retry_marker, &retry_pc);
    retry_no_fault = !excpt.triggered;

    goto_priv(PRIV_M);
    const bool compressed_retry_commits_once =
        retry_no_fault &&
        retry_pc.next_pc == retry_pc.pc + 2U &&
        read64(marker_paddr) == retry_marker &&
        read64(guard_before) == guard_before_seed &&
        read64(store_paddr) == retry_payload &&
        read64(guard_after) == guard_after_seed &&
        read64(store_paddr) != fault_payload;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("no-write C.SD fault keeps compressed fault PC, PC+2 resume marker, store tval and pending STIP separate",
        compressed_fault_metadata_ok,
        "cause=%lx epc=%lx pc=%lx next=%lx tval=%lx expected_tval=%lx marker=%lx pending=%lx traps=%lx",
        fault_cause, fault_epc, fault_pc.pc, fault_pc.next_pc,
        fault_tval, store_vaddr, read64(marker_paddr),
        pending_after_fault, ai_imc_trap_count);
    TEST_ASSERT("faulting compressed store does not leak payload bytes into target or adjacent guards before repair",
        compressed_fault_no_side_effect,
        "before=%lx target=%lx after=%lx",
        read64(guard_before), read64(store_paddr), read64(guard_after));
    TEST_ASSERT("the STIP preserved across compressed memory fault repair is delivered exactly once before retry",
        pending_stip_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("after PTE repair, retry C.SD uses a fresh compressed PC+2 path and writes the payload exactly once",
        compressed_retry_commits_once,
        "retry_pc=%lx next=%lx marker=%lx before=%lx target=%lx after=%lx",
        retry_pc.pc, retry_pc.next_pc, read64(marker_paddr),
        read64(guard_before), read64(store_paddr), read64(guard_after));

    TEST_END(ai_interrupt_memory_hs_stip_compressed_store_fault_pc2_retry_corner);
}

bool ai_interrupt_memory_m_stip_amo_w_d_width_owner_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0xa00U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0xa08U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xa10U;
    const uint64_t initial = 0x1122334480000010ULL;
    const uint32_t w_addend = 0x7ffffff0U;
    const uint64_t d_addend = 0x10ULL;
    const uint64_t after_w = 0x1122334400000000ULL;
    const uint64_t after_d = 0x1122334400000010ULL;
    const uint64_t guard_seed = 0x696d63616d6faa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t old_w = 0ULL;
    uint64_t old_d = 0ULL;

    goto_priv(PRIV_M);
    write64(target, initial);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    old_w = ai_imc_amoadd_w_raw64((uint32_t *)target, w_addend);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    old_d = ai_imc_amoadd_d((uint64_t *)target, d_addend);
    write64(marker, old_w ^ old_d ^ read64(target));
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool amo_width_contract =
        old_w == ai_imc_sign_extend_w((uint32_t)initial) &&
        old_d == after_w &&
        read64(target) == after_d &&
        read64(marker) == (old_w ^ old_d ^ after_d) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between AMO.W and AMO.D is logged once and clears the pending source",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("AMO.W returns the sign-extended lower word old value while AMO.D sees the full post-W doubleword owner",
        amo_width_contract,
        "old_w=%lx old_d=%lx final=%lx marker=%lx guard=%lx",
        old_w, old_d, read64(target), read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_amo_w_d_width_owner_corner);
}

bool ai_interrupt_memory_m_stip_lrsc_w_d_reservation_width_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0xa40U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0xa48U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xa50U;
    const uint64_t initial = 0x5566778880000010ULL;
    const uint32_t word_payload = 0xa5a55a5aU;
    const uint64_t double_payload = 0x13579bdf2468ace0ULL;
    const uint64_t word_success_image = 0x55667788a5a55a5aULL;
    const uint64_t guard_seed = 0x696d636c7277aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t lrw = 0ULL;
    uint64_t scw_status = 1ULL;
    uint64_t after_word_sc = 0ULL;
    uint64_t lrd = 0ULL;
    uint64_t scd_status = 1ULL;

    goto_priv(PRIV_M);
    write64(target, initial);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    lrw = ai_imc_lr_w_raw64((uint32_t *)target);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    scw_status = ai_imc_sc_w_raw64((uint32_t *)target, word_payload);
    after_word_sc = read64(target);
    lrd = ai_imc_lr_d((uint64_t *)target);
    scd_status = ai_imc_sc_d((uint64_t *)target, double_payload);
    write64(marker, lrw ^ scw_status ^ lrd ^ scd_status ^
        read64(target));
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool word_reservation_contract =
        lrw == ai_imc_sign_extend_w((uint32_t)initial) &&
        ((scw_status == 0ULL && after_word_sc == word_success_image) ||
         (scw_status != 0ULL && after_word_sc == initial));
    const bool double_reservation_contract =
        lrd == after_word_sc &&
        scd_status == 0ULL &&
        read64(target) == double_payload &&
        read64(marker) == (lrw ^ scw_status ^ lrd ^ scd_status ^
            double_payload) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between LR.W and SC.W is logged once and does not leave the timer pending",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("the W-width LR/SC pair uses the lower-word reservation contract without changing the upper word on success",
        word_reservation_contract,
        "lrw=%lx scw=%lx after_word=%lx initial=%lx word_success=%lx",
        lrw, scw_status, after_word_sc, initial, word_success_image);
    TEST_ASSERT("the later D-width LR/SC pair starts from the W-result image and commits the full doubleword once",
        double_reservation_contract,
        "lrd=%lx scd=%lx final=%lx marker=%lx guard=%lx",
        lrd, scd_status, read64(target), read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_lrsc_w_d_reservation_width_corner);
}

bool ai_interrupt_memory_m_stip_lrsc_amo_handler_same_target_owner_corner()
{
    TEST_START();

    const uintptr_t sc_target = phys_page_base(SCRATCHPAD) + 0xb80U;
    const uintptr_t amo_target = phys_page_base(SCRATCHPAD) + 0xb88U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0xb90U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xb98U;
    const uint64_t sc_seed = 0x696d636873637301ULL;
    const uint64_t sc_payload = 0x696d6368736373ffULL;
    const uint64_t sc_success_payload = 0x696d636873637355ULL;
    const uint64_t amo_seed = 0x696d6368616d6f01ULL;
    const uint64_t amo_addend = 0x55ULL;
    const uint64_t marker_value = 0x696d636873616d6bULL;
    const uint64_t guard_seed = 0x696d63687361aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t lr_value = 0ULL;
    uint64_t sc_status = 1ULL;
    uint64_t sc_final = 0ULL;
    uint64_t lr_success_value = 0ULL;
    uint64_t sc_success_status = 1ULL;
    uint64_t sc_success_final = 0ULL;
    uint64_t amo_old = 0ULL;
    uint64_t amo_final = 0ULL;

    goto_priv(PRIV_M);
    write64(sc_target, sc_seed);
    write64(amo_target, amo_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_handler_probe_addr = sc_target;
    lr_value = ai_imc_lr_d((uint64_t *)sc_target);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sc_status = ai_imc_sc_d((uint64_t *)sc_target, sc_payload);
    sc_final = read64(sc_target);

    write64(sc_target, sc_seed);
    fence_iorw_iorw();
    lr_success_value = ai_imc_lr_d((uint64_t *)sc_target);
    sc_success_status = ai_imc_sc_d((uint64_t *)sc_target,
        sc_success_payload);
    sc_success_final = read64(sc_target);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);

    ai_imc_handler_probe_addr = amo_target;
    amo_old = ai_imc_amoadd_d((uint64_t *)amo_target, amo_addend);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(3ULL, AI_IMC_SPIN_LIMIT);
    amo_final = read64(amo_target);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool interrupt_probe_logged =
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        ai_imc_trap_cause[2] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[2] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        ai_imc_handler_log_page[2] == 0x696d63686c6f6702ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool sc_event_set_consistent =
        lr_value == sc_seed &&
        ai_imc_handler_probe_value[0] == sc_seed &&
        ((sc_status == 0ULL && sc_final == sc_payload) ||
         (sc_status != 0ULL && sc_final == sc_seed));
    const bool sc_success_completion_owner =
        lr_success_value == sc_seed &&
        sc_success_status == 0ULL &&
        sc_success_final == sc_success_payload &&
        ai_imc_handler_probe_value[1] == sc_success_payload;
    const bool amo_same_target_owner =
        amo_old == amo_seed &&
        amo_final == amo_seed + amo_addend &&
        ai_imc_handler_probe_value[2] == amo_seed + amo_addend &&
        amo_final != ai_imc_handler_log_page[2];
    const bool marker_guard_ok =
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler reads the selected atomic target exactly once per episode and clears the pending source",
        interrupt_probe_logged,
        "trap_count=%lx cause0=%lx cause1=%lx cause2=%lx probe0=%lx probe1=%lx probe2=%lx log0=%lx log1=%lx log2=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_cause[2],
        ai_imc_handler_probe_value[0], ai_imc_handler_probe_value[1],
        ai_imc_handler_probe_value[2], ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1], ai_imc_handler_log_page[2],
        CSRR(CSR_MIP));
    TEST_ASSERT("LR/SC interrupted by a same-target handler load keeps status and final memory in the same legal event set",
        sc_event_set_consistent,
        "lr=%lx sc_status=%lx sc_final=%lx probe=%lx seed=%lx payload=%lx",
        lr_value, sc_status, sc_final, ai_imc_handler_probe_value[0],
        sc_seed, sc_payload);
    TEST_ASSERT("a fresh successful SC followed by a same-target handler load reports one coherent success generation",
        sc_success_completion_owner,
        "lr_success=%lx sc_success=%lx final=%lx probe=%lx payload=%lx",
        lr_success_value, sc_success_status, sc_success_final,
        ai_imc_handler_probe_value[1], sc_success_payload);
    TEST_ASSERT("AMO old-value and final memory remain owned by the mainline target even when the handler reads that same target",
        amo_same_target_owner,
        "amo_old=%lx amo_final=%lx probe=%lx expected_final=%lx handler_log=%lx",
        amo_old, amo_final, ai_imc_handler_probe_value[2],
        amo_seed + amo_addend, ai_imc_handler_log_page[2]);
    TEST_ASSERT("same-target handler probing does not clobber the independent marker or guard",
        marker_guard_ok,
        "marker=%lx guard=%lx", read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_lrsc_amo_handler_same_target_owner_corner);
}

bool ai_interrupt_memory_m_stip_lrsc_handler_amo_log_owner_corner()
{
    TEST_START();

    const uintptr_t lrsc_target = phys_page_base(SWITCH1) + 0x660U;
    const uintptr_t handler_amo = phys_page_base(SWITCH1) + 0x668U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x670U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x678U;
    const uint64_t lrsc_seed = 0x696d636c68616d01ULL;
    const uint64_t sc_payload = 0x696d636c68616d11ULL;
    const uint64_t fresh_seed = 0x696d636c68616d21ULL;
    const uint64_t fresh_payload = 0x696d636c68616d31ULL;
    const uint64_t handler_seed = 0x696d6368616d6f41ULL;
    const uint64_t handler_addend = 0x23ULL;
    const uint64_t marker_payload = 0x696d636c686d6172ULL;
    const uint64_t guard_seed = 0x696d636c6861aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t lr_value = 0ULL;
    uint64_t sc_status = 1ULL;
    uint64_t sc_final = 0ULL;
    uint64_t fresh_lr = 0ULL;
    uint64_t fresh_sc_status = 1ULL;

    goto_priv(PRIV_M);
    write64(lrsc_target, lrsc_seed);
    write64(handler_amo, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_amo;
    ai_imc_handler_amo_addend = handler_addend;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_amo_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    lr_value = ai_imc_lr_d((uint64_t *)lrsc_target);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sc_status = ai_imc_sc_d((uint64_t *)lrsc_target, sc_payload);
    sc_final = read64(lrsc_target);

    write64(lrsc_target, fresh_seed);
    fresh_lr = ai_imc_lr_d((uint64_t *)lrsc_target);
    fresh_sc_status = ai_imc_sc_d((uint64_t *)lrsc_target,
        fresh_payload);
    write64(marker, marker_payload);
    fence_iorw_iorw();

    const bool interrupt_handler_amo_exact =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_amo_old[0] == handler_seed &&
        ai_imc_handler_amo_final[0] == handler_seed + handler_addend &&
        ai_imc_handler_probe_value[0] == handler_seed + handler_addend &&
        read64(handler_amo) == handler_seed + handler_addend &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool interrupted_lrsc_event_set =
        lr_value == lrsc_seed &&
        ((sc_status == 0ULL && sc_final == sc_payload) ||
         (sc_status != 0ULL && sc_final == lrsc_seed)) &&
        sc_final != (handler_seed + handler_addend);
    const bool fresh_lrsc_recovers =
        fresh_lr == fresh_seed &&
        fresh_sc_status == 0ULL &&
        read64(lrsc_target) == fresh_payload &&
        read64(marker) == marker_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    ai_imc_handler_amo_addend = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler AMO log updates only the handler target once and clears the pending source",
        interrupt_handler_amo_exact,
        "trap_count=%lx cause=%lx old=%lx final=%lx probe=%lx mem=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_amo_old[0], ai_imc_handler_amo_final[0],
        ai_imc_handler_probe_value[0], read64(handler_amo),
        CSRR(CSR_MIP));
    TEST_ASSERT("mainline SC after an interrupt handler AMO keeps a legal reservation event set for the main target",
        interrupted_lrsc_event_set,
        "lr=%lx sc=%lx final=%lx seed=%lx payload=%lx handler_final=%lx",
        lr_value, sc_status, sc_final, lrsc_seed, sc_payload,
        ai_imc_handler_amo_final[0]);
    TEST_ASSERT("a fresh LR/SC after the handler AMO succeeds on the main target and preserves marker/guard memory",
        fresh_lrsc_recovers,
        "fresh_lr=%lx fresh_sc=%lx target=%lx marker=%lx guard=%lx",
        fresh_lr, fresh_sc_status, read64(lrsc_target),
        read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_lrsc_handler_amo_log_owner_corner);
}

bool ai_interrupt_memory_m_stip_load_amo_wb_owner_corner()
{
    TEST_START();

    const uintptr_t load_before_addr = phys_page_base(SWITCH1) + 0x6a0U;
    const uintptr_t amo_after_addr = phys_page_base(SWITCH1) + 0x6a8U;
    const uintptr_t amo_before_addr = phys_page_base(SWITCH1) + 0x6b0U;
    const uintptr_t load_after_addr = phys_page_base(SWITCH1) + 0x6b8U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x6c0U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x6c8U;
    const uint64_t load_before_seed = 0x696d636c64776201ULL;
    const uint64_t amo_after_seed = 0x696d63616d776202ULL;
    const uint64_t amo_after_addend = 0x35ULL;
    const uint64_t amo_before_seed = 0x696d63616d776203ULL;
    const uint64_t amo_before_addend = 0x57ULL;
    const uint64_t load_after_seed = 0x696d636c64776204ULL;
    const uint64_t guard_seed = 0x696d636c616daa05ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t load_before = 0ULL;
    uint64_t amo_after_old = 0ULL;
    uint64_t amo_before_old = 0ULL;
    uint64_t load_after = 0ULL;

    goto_priv(PRIV_M);
    write64(load_before_addr, load_before_seed);
    write64(amo_after_addr, amo_after_seed);
    write64(amo_before_addr, amo_before_seed);
    write64(load_after_addr, load_after_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    load_before = read64(load_before_addr);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    amo_after_old = ai_imc_amoadd_d((uint64_t *)amo_after_addr,
        amo_after_addend);

    amo_before_old = ai_imc_amoadd_d((uint64_t *)amo_before_addr,
        amo_before_addend);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    load_after = read64(load_after_addr);

    write64(marker, load_before ^ amo_after_old ^ amo_before_old ^
        load_after ^ read64(amo_after_addr) ^ read64(amo_before_addr));
    fence_iorw_iorw();

    const bool interrupt_logged_twice =
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool load_and_amo_wb_owners =
        load_before == load_before_seed &&
        load_after == load_after_seed &&
        amo_after_old == amo_after_seed &&
        amo_before_old == amo_before_seed &&
        read64(amo_after_addr) == amo_after_seed + amo_after_addend &&
        read64(amo_before_addr) == amo_before_seed + amo_before_addend &&
        read64(load_before_addr) == load_before_seed &&
        read64(load_after_addr) == load_after_seed &&
        load_before != amo_after_old &&
        load_after != amo_before_old &&
        amo_after_old != ai_imc_handler_log_page[0] &&
        amo_before_old != ai_imc_handler_log_page[1];
    const bool marker_guard_ok =
        read64(marker) == (load_before_seed ^ amo_after_seed ^
            amo_before_seed ^ load_after_seed ^
            (amo_after_seed + amo_after_addend) ^
            (amo_before_seed + amo_before_addend)) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("two STIP windows around load/AMO writeback episodes are logged once each and clear their source",
        interrupt_logged_twice,
        "trap_count=%lx c0=%lx c1=%lx mip0=%lx mip1=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_mip[0],
        ai_imc_trap_mip[1], CSRR(CSR_MIP));
    TEST_ASSERT("load rd values and AMO old-values keep separate owners across both interrupt-adjacent orders",
        load_and_amo_wb_owners,
        "load_before=%lx load_after=%lx amo_after_old=%lx amo_before_old=%lx amo_after_final=%lx amo_before_final=%lx",
        load_before, load_after, amo_after_old, amo_before_old,
        read64(amo_after_addr), read64(amo_before_addr));
    TEST_ASSERT("load/AMO owner checking leaves the independent marker and guard exactly-once",
        marker_guard_ok,
        "marker=%lx guard=%lx", read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_load_amo_wb_owner_corner);
}

bool ai_interrupt_memory_m_stip_misaligned_atomic_fault_pending_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0xc00U;
    const uintptr_t misaligned = target + 1U;
    const uintptr_t guard_lo = target - sizeof(uint64_t);
    const uintptr_t guard_hi = target + sizeof(uint64_t);
    const uintptr_t marker = target + 2U * sizeof(uint64_t);
    const uint64_t seed = 0x696d636d61746f6dULL;
    const uint64_t guard_lo_seed = 0x696d636d6174aa01ULL;
    const uint64_t guard_hi_seed = 0x696d636d6174aa02ULL;
    const uint64_t amo_addend = 0x11ULL;
    const uint64_t aligned_addend = 0x22ULL;
    const uint64_t stale_sc_payload = 0x696d636d61747363ULL;
    const uint64_t marker_value = 0x696d636d61746d6bULL;
    const uint64_t amo_sentinel = 0x696d636d61746fddULL;
    const uint64_t lr_sentinel = 0x696d636d6c7264ddULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t amo_fault_cause = 0ULL;
    uint64_t amo_fault_tval = 0ULL;
    uint64_t amo_fault_result = 0ULL;
    uint64_t lr_fault_cause = 0ULL;
    uint64_t lr_fault_tval = 0ULL;
    uint64_t lr_fault_result = 0ULL;
    uint64_t pending_after_faults = 0ULL;
    uint64_t stale_sc_status = 0ULL;
    uint64_t target_after_stale_sc = 0ULL;
    uint64_t aligned_amo_old = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("misaligned atomic target is deliberately unaligned while the base target remains naturally aligned",
        (target & 7ULL) == 0ULL && (misaligned & 7ULL) != 0ULL,
        "target=%lx misaligned=%lx", target, misaligned);
    write64(guard_lo, guard_lo_seed);
    write64(target, seed);
    write64(guard_hi, guard_hi_seed);
    write64(marker, 0ULL);
    fence_iorw_iorw();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    TEST_SETUP_EXCEPT();
    amo_fault_result = ai_imc_amoadd_d_addr(misaligned, amo_addend,
        amo_sentinel);
    amo_fault_cause = excpt.cause;
    amo_fault_tval = excpt.tval;

    TEST_SETUP_EXCEPT();
    lr_fault_result = ai_imc_lr_d_addr(misaligned, lr_sentinel);
    lr_fault_cause = excpt.cause;
    lr_fault_tval = excpt.tval;
    pending_after_faults = CSRR(CSR_MIP);

    TEST_SETUP_EXCEPT();
    stale_sc_status = ai_imc_sc_d_addr(target, stale_sc_payload);
    target_after_stale_sc = read64(target);

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    aligned_amo_old = ai_imc_amoadd_d((uint64_t *)target, aligned_addend);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool misaligned_atomic_faults_precise =
        amo_fault_cause == CAUSE_SAF &&
        amo_fault_tval == misaligned &&
        amo_fault_result == amo_sentinel &&
        lr_fault_cause == CAUSE_LAF &&
        lr_fault_tval == misaligned &&
        lr_fault_result == lr_sentinel &&
        (pending_after_faults & AI_IMC_MIP_STIP) != 0ULL;
    const bool no_atomic_side_effect_or_reservation =
        read64(guard_lo) == guard_lo_seed &&
        target_after_stale_sc == seed &&
        read64(guard_hi) == guard_hi_seed &&
        !excpt.triggered &&
        stale_sc_status != 0ULL;
    const bool interrupt_then_repair_amo_ok =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        aligned_amo_old == seed &&
        read64(target) == seed + aligned_addend &&
        read64(marker) == marker_value;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("misaligned AMO.D and LR.D report the project atomic access-fault class while STIP stays pending",
        misaligned_atomic_faults_precise,
        "amo_cause=%lx amo_tval=%lx amo_old=%lx lr_cause=%lx lr_tval=%lx lr_old=%lx pending=%lx",
        amo_fault_cause, amo_fault_tval, amo_fault_result,
        lr_fault_cause, lr_fault_tval, lr_fault_result,
        pending_after_faults);
    TEST_ASSERT("faulting atomic operations leave target/guards unchanged and do not create a usable reservation for stale SC.D",
        no_atomic_side_effect_or_reservation,
        "sc=%lx target_after_sc=%lx target_final=%lx guard_lo=%lx guard_hi=%lx",
        stale_sc_status, target_after_stale_sc, read64(target),
        read64(guard_lo), read64(guard_hi));
    TEST_ASSERT("after the fault window is skipped, the preserved STIP is delivered once and a fresh aligned AMO updates the target exactly once",
        interrupt_then_repair_amo_ok,
        "trap_count=%lx cause=%lx mip=%lx amo_old=%lx final=%lx marker=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0], CSRR(CSR_MIP),
        aligned_amo_old, read64(target), read64(marker));

    TEST_END(ai_interrupt_memory_m_stip_misaligned_atomic_fault_pending_corner);
}

bool ai_interrupt_memory_m_stip_pmp_lr_fault_handler_amo_owner_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH1;
    const uintptr_t vaddr = hs_page_base(target_page) + 0xe20U;
    const uintptr_t paddr = phys_page_base(target_page) + 0xe20U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t handler_amo = phys_page_base(SCRATCHPAD) + 0xc40U;
    const uintptr_t handler_guard = handler_amo + sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t target_seed = 0x696d636c72666175ULL;
    const uint64_t target_guard_seed = 0x696d636c7266aa01ULL;
    const uint64_t handler_seed = 0x696d6368616d6f20ULL;
    const uint64_t handler_addend = 0x33ULL;
    const uint64_t handler_guard_seed = 0x696d6368616daa02ULL;
    const uint64_t lr_sentinel = 0x696d636c7266dd01ULL;
    const uint64_t amo_sentinel = 0x696d63616d6fdd02ULL;
    const uint64_t amo_addend = 0x44ULL;
    const uint64_t stale_sc_payload = 0x696d637374616c65ULL;
    const uint64_t fresh_sc_payload = 0x696d636672657368ULL;
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
    uint64_t lr_fault_value = 0ULL;
    uint64_t lr_fault_cause = 0ULL;
    uint64_t lr_fault_tval = 0ULL;
    uint64_t amo_fault_old = 0ULL;
    uint64_t amo_fault_cause = 0ULL;
    uint64_t amo_fault_tval = 0ULL;
    uint64_t pending_after_atomic_faults = 0ULL;
    uint64_t stale_sc_status = 0ULL;
    uint64_t fresh_lr_value = 0ULL;
    uint64_t fresh_sc_status = 1ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, target_seed);
    write64(guard, target_guard_seed);
    write64(handler_amo, handler_seed);
    write64(handler_guard, handler_guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_R);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    lr_fault_value = ai_imc_lr_d_addr(vaddr, lr_sentinel);
    ai_imc_clear_mprv_translation();
    lr_fault_cause = excpt.cause;
    lr_fault_tval = excpt.tval;

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    amo_fault_old = ai_imc_amoadd_d_addr(vaddr, amo_addend,
        amo_sentinel);
    ai_imc_clear_mprv_translation();
    amo_fault_cause = excpt.cause;
    amo_fault_tval = excpt.tval;
    pending_after_atomic_faults = CSRR(CSR_MIP);

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_amo;
    ai_imc_handler_amo_addend = handler_addend;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_amo_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    stale_sc_status = ai_imc_sc_d_addr(vaddr, stale_sc_payload);
    const bool stale_sc_no_exception = !excpt.triggered;
    ai_imc_clear_mprv_translation();

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    fresh_lr_value = ai_imc_lr_d_addr(vaddr, lr_sentinel);
    fresh_sc_status = ai_imc_sc_d_addr(vaddr, fresh_sc_payload);
    const bool fresh_lrsc_no_exception = !excpt.triggered;
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();

    const bool main_faulting_atomic_suppressed =
        lr_fault_cause == CAUSE_LAF &&
        lr_fault_tval == vaddr &&
        lr_fault_value == lr_sentinel &&
        amo_fault_cause == CAUSE_SAF &&
        amo_fault_tval == vaddr &&
        amo_fault_old == amo_sentinel &&
        (pending_after_atomic_faults & AI_IMC_MIP_STIP) != 0ULL &&
        read64(paddr) == fresh_sc_payload &&
        read64(guard) == target_guard_seed;
    const bool handler_amo_uses_own_target =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_amo_old[0] == handler_seed &&
        ai_imc_handler_amo_final[0] == handler_seed + handler_addend &&
        ai_imc_handler_probe_value[0] == handler_seed + handler_addend &&
        read64(handler_amo) == handler_seed + handler_addend &&
        read64(handler_guard) == handler_guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool reservation_recovery_is_fresh =
        stale_sc_no_exception &&
        stale_sc_status != 0ULL &&
        fresh_lrsc_no_exception &&
        fresh_lr_value == target_seed &&
        fresh_sc_status == 0ULL &&
        read64(paddr) == fresh_sc_payload;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    ai_imc_handler_probe_addr = 0ULL;
    ai_imc_handler_amo_addend = 0ULL;
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("faulting LR.D and AMOADD.D refresh cause/tval and suppress main-target side effects while STIP remains pending",
        main_faulting_atomic_suppressed,
        "lr_cause=%lx lr_tval=%lx lr_val=%lx amo_cause=%lx amo_tval=%lx amo_old=%lx pending=%lx target=%lx guard=%lx",
        lr_fault_cause, lr_fault_tval, lr_fault_value,
        amo_fault_cause, amo_fault_tval, amo_fault_old,
        pending_after_atomic_faults, read64(paddr), read64(guard));
    TEST_ASSERT("the intervening STIP handler AMO updates only its own log target and clears the pending source exactly once",
        handler_amo_uses_own_target,
        "trap_count=%lx cause=%lx old=%lx final=%lx probe=%lx handler_mem=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_amo_old[0], ai_imc_handler_amo_final[0],
        ai_imc_handler_probe_value[0], read64(handler_amo),
        CSRR(CSR_MIP));
    TEST_ASSERT("after PMP repair, a stale SC.D still fails, while a fresh LR/SC pair commits the main target once",
        reservation_recovery_is_fresh,
        "stale_sc=%lx fresh_lr=%lx fresh_sc=%lx target=%lx expected=%lx",
        stale_sc_status, fresh_lr_value, fresh_sc_status, read64(paddr),
        fresh_sc_payload);

    TEST_END(ai_interrupt_memory_m_stip_pmp_lr_fault_handler_amo_owner_corner);
}

bool ai_interrupt_memory_m_stip_fault_class_rotation_retry_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t base_paddr = phys_page_base(target_page) + 0xf00U;
    const uintptr_t base_vaddr = hs_page_base(target_page) + 0xf00U;
    const uintptr_t atomic_paddr = base_paddr;
    const uintptr_t atomic_misaligned = atomic_paddr + 1U;
    const uintptr_t pf_vaddr = base_vaddr + 0x20U;
    const uintptr_t pf_paddr = base_paddr + 0x20U;
    const uintptr_t af_vaddr = base_vaddr + 0x28U;
    const uintptr_t af_paddr = base_paddr + 0x28U;
    const uintptr_t guard = base_paddr + 0x30U;
    const uintptr_t pmp_start = base_paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t atomic_seed = 0x696d636663617431ULL;
    const uint64_t pf_seed = 0x696d636663706631ULL;
    const uint64_t af_seed = 0x696d636663616631ULL;
    const uint64_t guard_seed = 0x696d63666372aa01ULL;
    const uint64_t amo_sentinel = 0x696d6366636174ddULL;
    const uint64_t amo_addend = 0x55ULL;
    const uint64_t final_pf_payload = 0x696d636663706f6bULL;
    const uint64_t final_af_payload = 0x696d636663616f6bULL;
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
    uint64_t amo_fault_old = 0ULL;
    uint64_t amo_fault_cause = 0ULL;
    uint64_t amo_fault_tval = 0ULL;
    uint64_t pf_load_value = 0ULL;
    uint64_t pf_cause = 0ULL;
    uint64_t pf_tval = 0ULL;
    uint64_t af_load_value = 0ULL;
    uint64_t af_cause = 0ULL;
    uint64_t af_tval = 0ULL;
    uint64_t final_pf_value = 0ULL;
    uint64_t final_af_value = 0ULL;

    goto_priv(PRIV_M);
    write64(atomic_paddr, atomic_seed);
    write64(pf_paddr, pf_seed);
    write64(af_paddr, af_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    TEST_SETUP_EXCEPT();
    amo_fault_old = ai_imc_amoadd_d_addr(atomic_misaligned, amo_addend,
        amo_sentinel);
    amo_fault_cause = excpt.cause;
    amo_fault_tval = excpt.tval;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_set_hs_leaf(target_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    pf_load_value = ai_imc_ld_probe(pf_vaddr, pf_seed ^ 0xffffULL);
    ai_imc_clear_mprv_translation();
    pf_cause = excpt.cause;
    pf_tval = excpt.tval;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_set_hs_leaf(target_page,
        PTE_V | PTE_U | PTE_AD | PTE_RWX);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_R);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    af_load_value = ai_imc_ld_probe(af_vaddr, af_seed ^ 0xffffULL);
    ai_imc_clear_mprv_translation();
    af_cause = excpt.cause;
    af_tval = excpt.tval;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(3ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_RWX);
    ai_imc_set_hs_leaf(target_page,
        PTE_V | PTE_U | PTE_AD | PTE_RWX);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    final_pf_value = ai_imc_ld_probe(pf_vaddr, 0ULL);
    sd(pf_vaddr, final_pf_payload);
    final_af_value = ai_imc_ld_probe(af_vaddr, 0ULL);
    sd(af_vaddr, final_af_payload);
    const bool final_retry_no_exception = !excpt.triggered;
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();

    const bool class_and_tval_rotated =
        amo_fault_cause == CAUSE_SAF &&
        amo_fault_tval == atomic_misaligned &&
        amo_fault_old == amo_sentinel &&
        pf_cause == CAUSE_LPF &&
        pf_tval == pf_vaddr &&
        pf_load_value == (pf_seed ^ 0xffffULL) &&
        af_cause == CAUSE_LAF &&
        af_tval == af_vaddr &&
        af_load_value == (af_seed ^ 0xffffULL);
    const bool interrupts_between_rounds_exact =
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        ai_imc_trap_cause[2] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[2] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool repair_success_once =
        final_retry_no_exception &&
        final_pf_value == pf_seed &&
        final_af_value == af_seed &&
        read64(atomic_paddr) == atomic_seed &&
        read64(pf_paddr) == final_pf_payload &&
        read64(af_paddr) == final_af_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_set_hs_leaf(target_page,
        PTE_V | PTE_U | PTE_AD | PTE_RWX);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("misaligned atomic, page fault, and PMP access fault each refresh their own cause/tval instead of reusing stale metadata",
        class_and_tval_rotated,
        "amo=%lx/%lx pf=%lx/%lx af=%lx/%lx old=%lx pf_val=%lx af_val=%lx",
        amo_fault_cause, amo_fault_tval, pf_cause, pf_tval,
        af_cause, af_tval, amo_fault_old, pf_load_value,
        af_load_value);
    TEST_ASSERT("STIP is delivered exactly once between each fault-class repair round and clears its source",
        interrupts_between_rounds_exact,
        "trap_count=%lx c0=%lx c1=%lx c2=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_cause[2],
        CSRR(CSR_MIP));
    TEST_ASSERT("after all repairs, the load/store retry path commits only the final legal payloads and preserves atomic/guard memory",
        repair_success_once,
        "pf_value=%lx af_value=%lx atomic=%lx pf=%lx af=%lx guard=%lx",
        final_pf_value, final_af_value, read64(atomic_paddr),
        read64(pf_paddr), read64(af_paddr), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_fault_class_rotation_retry_corner);
}

bool ai_interrupt_memory_m_stip_vector_active_mask_store_load_corner()
{
    TEST_START();

    const uintptr_t old_src = phys_page_base(SWITCH1) + 0xb20U;
    const uintptr_t new_src = phys_page_base(SWITCH1) + 0xb60U;
    const uintptr_t dump = phys_page_base(SWITCH1) + 0xba0U;
    const uintptr_t target = phys_page_base(SWITCH1) + 0xbe0U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xc20U;
    const uint32_t old_words[AI_IMC_VEC_VL6] = {
        0x10101010U, 0x20202020U, 0x30303030U,
        0x40404040U, 0x50505050U, 0x60606060U
    };
    const uint32_t new_words[AI_IMC_VEC_VL6] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0xc3c3c3c3U,
        0xd4d4d4d4U, 0xe5e5e5e5U, 0xf6f6f6f6U
    };
    const uint32_t expected[AI_IMC_VEC_VL6] = {
        0xa1a1a1a1U, 0x20202020U, 0xc3c3c3c3U,
        0x40404040U, 0x50505050U, 0xf6f6f6f6U
    };
    const uint64_t guard_seed = 0x696d637661630001ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t load_vstart = 0xffffffffffffffffULL;
    uint64_t store_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL6; ++i) {
        write32(old_src + i * sizeof(uint32_t), old_words[i]);
        write32(new_src + i * sizeof(uint32_t), new_words[i]);
        write32(dump + i * sizeof(uint32_t), 0xeeeeeeeeU);
        write32(target + i * sizeof(uint32_t), old_words[i]);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_vec_active_025_load_dump((const uint32_t *)old_src,
        (const uint32_t *)new_src, (uint32_t *)dump, &load_vstart);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    ai_imc_vec_active_025_store((const uint32_t *)new_src,
        (uint32_t *)target, &store_vstart);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool active_load_store_image =
        load_vstart == 0ULL &&
        store_vstart == 0ULL &&
        ai_imc_words_match(dump, expected, AI_IMC_VEC_VL6) &&
        ai_imc_words_match(target, expected, AI_IMC_VEC_VL6) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between masked vector load and masked vector store is logged exactly once and cleared",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("active vector elements 0/2/5 update while inactive elements keep the old image and vstart clears after success",
        active_load_store_image,
        "load_vstart=%lx store_vstart=%lx dump=%x,%x,%x,%x,%x,%x target=%x,%x,%x,%x,%x,%x guard=%lx",
        load_vstart, store_vstart,
        read32(dump), read32(dump + 4U), read32(dump + 8U),
        read32(dump + 12U), read32(dump + 16U), read32(dump + 20U),
        read32(target), read32(target + 4U), read32(target + 8U),
        read32(target + 12U), read32(target + 16U),
        read32(target + 20U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_active_mask_store_load_corner);
}

bool ai_interrupt_memory_m_stip_vector_store_inactive_final_two_epoch_corner()
{
    TEST_START();

    const uintptr_t src0 = phys_page_base(SCRATCHPAD) + 0xf00U;
    const uintptr_t src1 = phys_page_base(SCRATCHPAD) + 0xf40U;
    const uintptr_t target0 = phys_page_base(SCRATCHPAD) + 0xf80U;
    const uintptr_t target1 = phys_page_base(SCRATCHPAD) + 0xfc0U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0xe00U;
    const uint32_t sentinel0[AI_IMC_VEC_VL6] = {
        0x70100000U, 0x70100001U, 0x70100002U,
        0x70100003U, 0x70100004U, 0x70100005U
    };
    const uint32_t sentinel1[AI_IMC_VEC_VL6] = {
        0x70200000U, 0x70200001U, 0x70200002U,
        0x70200003U, 0x70200004U, 0x70200005U
    };
    const uint32_t payload0[AI_IMC_VEC_VL6] = {
        0xa7100000U, 0xa7100001U, 0xa7100002U,
        0xa7100003U, 0xa7100004U, 0xa7100005U
    };
    const uint32_t payload1[AI_IMC_VEC_VL6] = {
        0xb7200000U, 0xb7200001U, 0xb7200002U,
        0xb7200003U, 0xb7200004U, 0xb7200005U
    };
    const uint32_t expected0[AI_IMC_VEC_VL6] = {
        0xa7100000U, 0x70100001U, 0xa7100002U,
        0x70100003U, 0xa7100004U, 0x70100005U
    };
    const uint32_t expected1[AI_IMC_VEC_VL6] = {
        0x70200000U, 0xb7200001U, 0x70200002U,
        0xb7200003U, 0xb7200004U, 0x70200005U
    };
    const uint64_t guard_seed = 0x696d6376696e6163ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t vstart0 = 0xffffffffffffffffULL;
    uint64_t vstart1 = 0xffffffffffffffffULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL6; ++i) {
        write32(src0 + i * sizeof(uint32_t), payload0[i]);
        write32(src1 + i * sizeof(uint32_t), payload1[i]);
        write32(target0 + i * sizeof(uint32_t), sentinel0[i]);
        write32(target1 + i * sizeof(uint32_t), sentinel1[i]);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_store_mask_024((const uint32_t *)src0,
        (uint32_t *)target0, &vstart0);
    const bool first_store_no_fault = !excpt.triggered;

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_store_mask_134((const uint32_t *)src1,
        (uint32_t *)target1, &vstart1);
    const bool second_store_no_fault = !excpt.triggered;
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool inactive_final_epoch0_ok =
        first_store_no_fault &&
        vstart0 == 0ULL &&
        ai_imc_words_match(target0, expected0, AI_IMC_VEC_VL6) &&
        read32(target0 + 20U) == sentinel0[5] &&
        read64(guard) == guard_seed;
    const bool adjacent_epoch1_ok =
        second_store_no_fault &&
        vstart1 == 0ULL &&
        ai_imc_words_match(target1, expected1, AI_IMC_VEC_VL6) &&
        ai_imc_words_match(target0, expected0, AI_IMC_VEC_VL6) &&
        read32(target1) == sentinel1[0] &&
        read32(target1 + 20U) == sentinel1[5] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between masked vector store epochs is logged exactly once and clears the pending source",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("first masked vector store keeps inactive final lane sentinel while all active lanes commit once",
        inactive_final_epoch0_ok,
        "fault=%d vstart=%lx target0=%x,%x,%x,%x,%x,%x guard=%lx",
        first_store_no_fault, vstart0,
        read32(target0), read32(target0 + 4U), read32(target0 + 8U),
        read32(target0 + 12U), read32(target0 + 16U),
        read32(target0 + 20U), read64(guard));
    TEST_ASSERT("adjacent post-interrupt vector store keeps its own payload/mask and does not overwrite the first epoch",
        adjacent_epoch1_ok,
        "fault=%d vstart=%lx target1=%x,%x,%x,%x,%x,%x target0_lane4=%x guard=%lx",
        second_store_no_fault, vstart1,
        read32(target1), read32(target1 + 4U), read32(target1 + 8U),
        read32(target1 + 12U), read32(target1 + 16U),
        read32(target1 + 20U), read32(target0 + 16U),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_store_inactive_final_two_epoch_corner);
}

bool ai_interrupt_memory_m_stip_vector_scalar_mixed_lsq_owner_corner()
{
    TEST_START();

    const uintptr_t vec_src0 = phys_page_base(SWITCH1) + 0xc00U;
    const uintptr_t vec_dst0 = phys_page_base(SWITCH1) + 0xc20U;
    const uintptr_t scalar_load_addr = phys_page_base(SWITCH1) + 0xc40U;
    const uintptr_t scalar_store_addr = phys_page_base(SWITCH1) + 0xc48U;
    const uintptr_t vec_src1 = phys_page_base(SWITCH1) + 0xc60U;
    const uintptr_t vec_dst1 = phys_page_base(SWITCH1) + 0xc80U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0xca0U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xca8U;
    const uint32_t v0_0 = 0x6d765330U;
    const uint32_t v0_1 = 0x6d765331U;
    const uint32_t v0_2 = 0x6d765332U;
    const uint32_t v0_3 = 0x6d765333U;
    const uint32_t v1_0 = 0x6d765731U;
    const uint32_t v1_1 = 0x6d765732U;
    const uint32_t v1_2 = 0x6d765733U;
    const uint32_t v1_3 = 0x6d765734U;
    const uint64_t scalar_seed = 0x696d63766c737101ULL;
    const uint64_t scalar_payload = 0x696d63766c7372ffULL;
    const uint64_t guard_seed = 0x696d63766c73aa01ULL;
    const uint64_t marker_value = 0x696d63766c736d01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t vstart0 = 0xffffffffffffffffULL;
    uint64_t vstart1 = 0xffffffffffffffffULL;
    uint64_t scalar_loaded = 0ULL;

    goto_priv(PRIV_M);
    write32(vec_src0, v0_0);
    write32(vec_src0 + 4U, v0_1);
    write32(vec_src0 + 8U, v0_2);
    write32(vec_src0 + 12U, v0_3);
    write32(vec_src1, v1_0);
    write32(vec_src1 + 4U, v1_1);
    write32(vec_src1 + 8U, v1_2);
    write32(vec_src1 + 12U, v1_3);
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        write32(vec_dst0 + i * sizeof(uint32_t), 0xaaaa0000U + i);
        write32(vec_dst1 + i * sizeof(uint32_t), 0xbbbb0000U + i);
    }
    write64(scalar_load_addr, scalar_seed);
    write64(scalar_store_addr, 0ULL);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_vec_prepare_e32m1_vl4();
    ai_imc_vec_copy_current_vl_u32((const uint32_t *)vec_src0,
        (uint32_t *)vec_dst0, &vstart0);
    scalar_loaded = read64(scalar_load_addr);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(scalar_store_addr, scalar_payload);
    ai_imc_vec_copy_current_vl_u32((const uint32_t *)vec_src1,
        (uint32_t *)vec_dst1, &vstart1);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool scalar_owner_ok =
        scalar_loaded == scalar_seed &&
        read64(scalar_store_addr) == scalar_payload &&
        read64(scalar_store_addr) != ai_imc_handler_log_page[0] &&
        read64(scalar_load_addr) == scalar_seed;
    const bool vector_owner_ok =
        vstart0 == 0ULL &&
        vstart1 == 0ULL &&
        ai_imc_words_match4(vec_dst0, v0_0, v0_1, v0_2, v0_3) &&
        ai_imc_words_match4(vec_dst1, v1_0, v1_1, v1_2, v1_3) &&
        read32(vec_dst0) != (uint32_t)scalar_payload &&
        read32(vec_dst1) != (uint32_t)scalar_seed;
    const bool marker_guard_ok =
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between mixed scalar/vector memory episodes is logged exactly once and clears the pending source",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx handler_log=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("scalar load/store payloads remain scalar-owned after vector memory and handler traffic",
        scalar_owner_ok,
        "scalar_load=%lx scalar_target=%lx seed=%lx payload=%lx handler_log=%lx",
        scalar_loaded, read64(scalar_store_addr), scalar_seed,
        scalar_payload, ai_imc_handler_log_page[0]);
    TEST_ASSERT("vector destination groups keep their own element payloads and never receive scalar/handler data",
        vector_owner_ok,
        "vstart0=%lx vstart1=%lx dst0=%x,%x,%x,%x dst1=%x,%x,%x,%x",
        vstart0, vstart1,
        read32(vec_dst0), read32(vec_dst0 + 4U),
        read32(vec_dst0 + 8U), read32(vec_dst0 + 12U),
        read32(vec_dst1), read32(vec_dst1 + 4U),
        read32(vec_dst1 + 8U), read32(vec_dst1 + 12U));
    TEST_ASSERT("mixed scalar/vector pressure leaves independent marker and guard memory intact",
        marker_guard_ok,
        "marker=%lx guard=%lx", read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_scalar_mixed_lsq_owner_corner);
}

bool ai_interrupt_memory_m_stip_vector_maskedoff_invalid_no_fault_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH2) + 0xb20U;
    const uintptr_t offsets = phys_page_base(SWITCH2) + 0xba0U;
    const uintptr_t old_src = phys_page_base(SWITCH2) + 0xbe0U;
    const uintptr_t src = phys_page_base(SWITCH2) + 0xc20U;
    const uintptr_t dump = phys_page_base(SWITCH2) + 0xc60U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0xca0U;
    const uint32_t old_words[AI_IMC_VEC_VL4] = {
        0x11111111U, 0x22222222U, 0x33333333U, 0x44444444U
    };
    const uint32_t src_words[AI_IMC_VEC_VL4] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0xc3c3c3c3U, 0xd4d4d4d4U
    };
    const uint32_t expected[AI_IMC_VEC_VL4] = {
        0xa1a1a1a1U, 0x22222222U, 0xc3c3c3c3U, 0x44444444U
    };
    const uint64_t invalid_offset =
        (uint64_t)(1ULL - (uint64_t)base);
    const uint64_t guard_seed = 0x696d63766d6faa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t load_vstart = 0xffffffffffffffffULL;
    uint64_t store_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        write8(base + i, 0x6dU);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(old_src + i * sizeof(uint32_t), old_words[i]);
        write32(src + i * sizeof(uint32_t), src_words[i]);
        write32(dump + i * sizeof(uint32_t), 0xeeeeeeeeU);
    }
    write64(offsets, 0ULL);
    write64(offsets + 8U, invalid_offset);
    write64(offsets + 16U, 8ULL);
    write64(offsets + 24U, invalid_offset);
    write32(base, src_words[0]);
    write32(base + 4U, old_words[1]);
    write32(base + 8U, src_words[2]);
    write32(base + 12U, old_words[3]);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_masked_indexed_even_load((const uint8_t *)base,
        (const uint64_t *)offsets, (const uint32_t *)old_src,
        (uint32_t *)dump, &load_vstart);
    const bool load_no_fault = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_masked_indexed_even_store((const uint32_t *)src,
        (const uint64_t *)offsets, (uint8_t *)base, &store_vstart);
    const bool store_no_fault = !excpt.triggered;
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool masked_invalid_suppressed =
        load_no_fault &&
        store_no_fault &&
        load_vstart == 0ULL &&
        store_vstart == 0ULL &&
        ai_imc_words_match(dump, expected, AI_IMC_VEC_VL4) &&
        read32(base) == expected[0] &&
        read32(base + 4U) == expected[1] &&
        read32(base + 8U) == expected[2] &&
        read32(base + 12U) == expected[3] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP around masked indexed vector memory is logged once without creating a ghost reentry",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("masked-off indexed lanes with invalid offsets produce no trap, no side effect, and active lanes keep their payload owner",
        masked_invalid_suppressed,
        "load_no_fault=%d store_no_fault=%d lvstart=%lx svstart=%lx dump=%x,%x,%x,%x base=%x,%x,%x,%x guard=%lx",
        load_no_fault, store_no_fault, load_vstart, store_vstart,
        read32(dump), read32(dump + 4U), read32(dump + 8U),
        read32(dump + 12U), read32(base), read32(base + 4U),
        read32(base + 8U), read32(base + 12U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_maskedoff_invalid_no_fault_corner);
}

bool ai_interrupt_memory_m_stip_vector_tail_policy_store_load_corner()
{
    TEST_START();

    const uintptr_t old_src = phys_page_base(SCRATCHPAD) + 0xb20U;
    const uintptr_t new_src = phys_page_base(SCRATCHPAD) + 0xb60U;
    const uintptr_t tu_dump = phys_page_base(SCRATCHPAD) + 0xba0U;
    const uintptr_t ta_dump = phys_page_base(SCRATCHPAD) + 0xbe0U;
    const uintptr_t store_target = phys_page_base(SCRATCHPAD) + 0xc20U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xc60U;
    const uint32_t old_words[AI_IMC_VEC_VL4] = {
        0x10101010U, 0x20202020U, 0x30303030U, 0x40404040U
    };
    const uint32_t new_words[AI_IMC_VEC_VL4] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0xc3c3c3c3U, 0xd4d4d4d4U
    };
    const uint32_t tail_undisturbed_expected[AI_IMC_VEC_VL4] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0x30303030U, 0x40404040U
    };
    const uint64_t guard_seed = 0x696d637674616101ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t tu_vstart = 0xffffffffffffffffULL;
    uint64_t ta_vstart = 0xffffffffffffffffULL;
    uint64_t store_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(old_src + i * sizeof(uint32_t), old_words[i]);
        write32(new_src + i * sizeof(uint32_t), new_words[i]);
        write32(tu_dump + i * sizeof(uint32_t), 0xeeeeeeeeU);
        write32(ta_dump + i * sizeof(uint32_t), 0xddddddddU);
        write32(store_target + i * sizeof(uint32_t), old_words[i]);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    ai_imc_vec_tail_load_dump((const uint32_t *)old_src,
        (const uint32_t *)new_src, (uint32_t *)tu_dump, 2ULL, 0ULL,
        &tu_vstart);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    ai_imc_vec_tail_load_dump((const uint32_t *)old_src,
        (const uint32_t *)new_src, (uint32_t *)ta_dump, 2ULL, 1ULL,
        &ta_vstart);
    ai_imc_vec_short_store((const uint32_t *)new_src,
        (uint32_t *)store_target, 2ULL, &store_vstart);
    fence_iorw_iorw();

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool tail_undisturbed_ok =
        tu_vstart == 0ULL &&
        ai_imc_words_match(tu_dump, tail_undisturbed_expected,
            AI_IMC_VEC_VL4);
    const bool tail_agnostic_allowed =
        ta_vstart == 0ULL &&
        read32(ta_dump) == new_words[0] &&
        read32(ta_dump + 4U) == new_words[1] &&
        (read32(ta_dump + 8U) == old_words[2] ||
         read32(ta_dump + 8U) == 0xffffffffU) &&
        (read32(ta_dump + 12U) == old_words[3] ||
         read32(ta_dump + 12U) == 0xffffffffU);
    const bool short_store_ok =
        store_vstart == 0ULL &&
        ai_imc_words_match(store_target, tail_undisturbed_expected,
            AI_IMC_VEC_VL4) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP between vector tail-policy rounds is logged once and clears before the short store",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("tail-undisturbed vector load and short vector store update only body elements while preserving tail memory",
        tail_undisturbed_ok && short_store_ok,
        "tu_vstart=%lx store_vstart=%lx tu=%x,%x,%x,%x store=%x,%x,%x,%x guard=%lx",
        tu_vstart, store_vstart,
        read32(tu_dump), read32(tu_dump + 4U),
        read32(tu_dump + 8U), read32(tu_dump + 12U),
        read32(store_target), read32(store_target + 4U),
        read32(store_target + 8U), read32(store_target + 12U),
        read64(guard));
    TEST_ASSERT("tail-agnostic vector load keeps active body elements and never replaces tail with handler magic",
        tail_agnostic_allowed,
        "ta_vstart=%lx ta=%x,%x,%x,%x",
        ta_vstart, read32(ta_dump), read32(ta_dump + 4U),
        read32(ta_dump + 8U), read32(ta_dump + 12U));

    TEST_END(ai_interrupt_memory_m_stip_vector_tail_policy_store_load_corner);
}

bool ai_interrupt_memory_m_stip_vector_indexed_unordered_owner_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH1) + 0xc80U;
    const uintptr_t offsets = phys_page_base(SWITCH1) + 0xd00U;
    const uintptr_t src = phys_page_base(SWITCH1) + 0xd40U;
    const uintptr_t dump = phys_page_base(SWITCH1) + 0xd80U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xdc0U;
    const uint64_t offset_words[AI_IMC_VEC_VL4] = {
        12ULL, 0ULL, 8ULL, 4ULL
    };
    const uint32_t src_words[AI_IMC_VEC_VL4] = {
        0xa0a0a003U, 0xb0b0b000U, 0xc0c0c002U, 0xd0d0d001U
    };
    const uint32_t expected_dump[AI_IMC_VEC_VL4] = {
        0xa0a0a003U, 0xb0b0b000U, 0xc0c0c002U, 0xd0d0d001U
    };
    const uint32_t expected_mem[AI_IMC_VEC_VL4] = {
        0xb0b0b000U, 0xd0d0d001U, 0xc0c0c002U, 0xa0a0a003U
    };
    const uint64_t guard_seed = 0x696d637669647801ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t final_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write64(offsets + i * sizeof(uint64_t), offset_words[i]);
        write32(src + i * sizeof(uint32_t), src_words[i]);
        write32(dump + i * sizeof(uint32_t), 0xeeeeeeeeU);
        write32(base + i * sizeof(uint32_t), 0x11110000U + (uint32_t)i);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_indexed_unordered_roundtrip((const uint8_t *)base,
        (const uint64_t *)offsets, (const uint32_t *)src,
        (uint32_t *)dump, &final_vstart);
    const bool no_fault = !excpt.triggered;

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool indexed_owner_ok =
        no_fault &&
        final_vstart == 0ULL &&
        ai_imc_words_match(dump, expected_dump, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(base, expected_mem, AI_IMC_VEC_VL4) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP before unordered indexed vector memory is logged exactly once and clears before owner checking",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("unordered indexed vector store/load keeps each lane bound to its explicit offset and does not swap owner after interrupt return",
        indexed_owner_ok,
        "no_fault=%d vstart=%lx dump=%x,%x,%x,%x mem=%x,%x,%x,%x guard=%lx",
        no_fault, final_vstart,
        read32(dump), read32(dump + 4U), read32(dump + 8U),
        read32(dump + 12U), read32(base), read32(base + 4U),
        read32(base + 8U), read32(base + 12U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_indexed_unordered_owner_corner);
}

bool ai_interrupt_memory_m_stip_vector_strided_stride_owner_corner()
{
    TEST_START();

    const uintptr_t load_mem = phys_page_base(SWITCH2) + 0xcc0U;
    const uintptr_t store_mem = phys_page_base(SWITCH2) + 0xd00U;
    const uintptr_t store_src = phys_page_base(SWITCH2) + 0xd40U;
    const uintptr_t dump = phys_page_base(SWITCH2) + 0xd80U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0xdc0U;
    const uint16_t load_words[AI_IMC_VEC_VL8] = {
        0xa100U, 0xa211U, 0xa322U, 0xa433U,
        0xa544U, 0xa655U, 0xa766U, 0xa877U
    };
    const uint16_t store_words[AI_IMC_VEC_VL8] = {
        0xb100U, 0xb211U, 0xb322U, 0xb433U,
        0xb544U, 0xb655U, 0xb766U, 0xb877U
    };
    const uint16_t load_expected[AI_IMC_VEC_VL8] = {
        0xa877U, 0xa766U, 0xa655U, 0xa544U,
        0xa433U, 0xa322U, 0xa211U, 0xa100U
    };
    const uint64_t guard_seed = 0x696d637673747201ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t final_vstart = 0xffffffffffffffffULL;
    bool dump_ok = true;
    bool store_ok = true;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL8; ++i) {
        write16(load_mem + i * sizeof(uint16_t), load_words[i]);
        write16(store_mem + i * sizeof(uint16_t), 0x7777U);
        write16(store_src + i * sizeof(uint16_t), store_words[i]);
        write16(dump + i * sizeof(uint16_t), 0xeeeeU);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_negative_stride_roundtrip(
        (const uint16_t *)(load_mem + (AI_IMC_VEC_VL8 - 1U) *
            sizeof(uint16_t)),
        (const uint16_t *)store_src,
        (uint16_t *)(store_mem + (AI_IMC_VEC_VL8 - 1U) *
            sizeof(uint16_t)),
        (uint16_t *)dump, &final_vstart);
    const bool no_fault = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL8; ++i) {
        if (read16(dump + i * sizeof(uint16_t)) != load_expected[i]) {
            dump_ok = false;
        }
        if (read16(store_mem + i * sizeof(uint16_t)) !=
            store_words[AI_IMC_VEC_VL8 - 1ULL - i]) {
            store_ok = false;
        }
    }

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool stride_owner_ok =
        no_fault &&
        final_vstart == 0ULL &&
        dump_ok &&
        store_ok &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP after negative-stride vector memory is logged exactly once and cleared",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("negative-stride vector load/store keeps descending element address ownership after the interrupt window",
        stride_owner_ok,
        "no_fault=%d vstart=%lx dump_ok=%d store_ok=%d dump0=%x dump7=%x store0=%x store7=%x guard=%lx",
        no_fault, final_vstart, dump_ok, store_ok,
        read16(dump), read16(dump + 14U), read16(store_mem),
        read16(store_mem + 14U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_strided_stride_owner_corner);
}

bool ai_interrupt_memory_m_stip_vector_segment_field_owner_corner()
{
    TEST_START();

    const uintptr_t src = phys_page_base(SCRATCHPAD) + 0xcc0U;
    const uintptr_t dst = phys_page_base(SCRATCHPAD) + 0xd00U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xd40U;
    const uint8_t pattern[AI_IMC_VEC_SEG2_BYTES] = {
        0x10U, 0x81U, 0x21U, 0x92U,
        0x32U, 0xa3U, 0x43U, 0xb4U
    };
    const uint64_t guard_seed = 0x696d637673656701ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t final_vstart = 0xffffffffffffffffULL;
    uint64_t bad_index = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_SEG2_BYTES; ++i) {
        write8(src + i, pattern[i]);
        write8(dst + i, 0xeeU);
    }
    write8(dst - 1U, 0xa5U);
    write8(dst + AI_IMC_VEC_SEG2_BYTES, 0x5aU);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e8_roundtrip((const uint8_t *)src, (uint8_t *)dst,
        &final_vstart);
    const bool no_fault = !excpt.triggered;

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool segment_owner_ok =
        no_fault &&
        final_vstart == 0ULL &&
        ai_imc_bytes_match_array(dst, pattern, AI_IMC_VEC_SEG2_BYTES,
            &bad_index) &&
        read8(dst - 1U) == 0xa5U &&
        read8(dst + AI_IMC_VEC_SEG2_BYTES) == 0x5aU &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP before segmented vector memory is logged once and does not reenter after source clear",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("vlseg2e8/vsseg2e8 keeps each field byte in element-major order and preserves neighboring guards",
        segment_owner_ok,
        "no_fault=%d vstart=%lx bad=%lx dst=%x,%x,%x,%x,%x,%x,%x,%x guard_lo=%x guard_hi=%x marker=%lx",
        no_fault, final_vstart, bad_index,
        read8(dst), read8(dst + 1U), read8(dst + 2U), read8(dst + 3U),
        read8(dst + 4U), read8(dst + 5U), read8(dst + 6U),
        read8(dst + 7U), read8(dst - 1U),
        read8(dst + AI_IMC_VEC_SEG2_BYTES), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_segment_field_owner_corner);
}

bool ai_interrupt_memory_hs_ssip_vector_segment_handler_scalar_probe_two_epoch_corner()
{
    TEST_START();

    const enum test_page segment_page = SWITCH2;
    const enum test_page handler_page = SCRATCHPAD;
    const uintptr_t src0_vaddr = hs_page_base(segment_page) + 0xe40U;
    const uintptr_t src1_vaddr = hs_page_base(segment_page) + 0xe80U;
    const uintptr_t dst0_vaddr = hs_page_base(segment_page) + 0xec0U;
    const uintptr_t dst1_vaddr = hs_page_base(segment_page) + 0xf00U;
    const uintptr_t handler_probe_vaddr = hs_page_base(handler_page) + 0xe90U;
    const uintptr_t src0_paddr = phys_page_base(segment_page) + 0xe40U;
    const uintptr_t src1_paddr = phys_page_base(segment_page) + 0xe80U;
    const uintptr_t dst0_paddr = phys_page_base(segment_page) + 0xec0U;
    const uintptr_t dst1_paddr = phys_page_base(segment_page) + 0xf00U;
    const uintptr_t handler_probe_paddr = phys_page_base(handler_page) + 0xe90U;
    const uintptr_t guard = phys_page_base(handler_page) + 0xe98U;
    const uint8_t pattern0[AI_IMC_VEC_SEG2_BYTES] = {
        0x11U, 0xa1U, 0x22U, 0xb2U,
        0x33U, 0xc3U, 0x44U, 0xd4U
    };
    const uint8_t pattern1[AI_IMC_VEC_SEG2_BYTES] = {
        0x55U, 0xe5U, 0x66U, 0xf6U,
        0x77U, 0x87U, 0x88U, 0x98U
    };
    const uint64_t handler_seed = 0x696d637673656768ULL;
    const uint64_t guard_seed = 0x696d6376736567aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    uint64_t vstart0 = 0xffffffffffffffffULL;
    uint64_t vstart1 = 0xffffffffffffffffULL;
    uint64_t bad_index0 = 0ULL;
    uint64_t bad_index1 = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_SEG2_BYTES; ++i) {
        write8(src0_paddr + i, pattern0[i]);
        write8(src1_paddr + i, pattern1[i]);
        write8(dst0_paddr + i, 0x70U + (uint8_t)i);
        write8(dst1_paddr + i, 0x90U + (uint8_t)i);
    }
    write8(dst0_paddr - 1U, 0xa5U);
    write8(dst0_paddr + AI_IMC_VEC_SEG2_BYTES, 0x5aU);
    write8(dst1_paddr - 1U, 0x3cU);
    write8(dst1_paddr + AI_IMC_VEC_SEG2_BYTES, 0xc3U);
    write64(handler_probe_paddr, handler_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(segment_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(handler_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRW(CSR_STVEC, (uintptr_t)ai_imc_stvec_same_target_load_handler);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e8_roundtrip((const uint8_t *)src0_vaddr,
        (uint8_t *)dst0_vaddr, &vstart0);
    const bool first_segment_no_fault = !excpt.triggered;

    ai_imc_handler_probe_addr = handler_probe_vaddr;
    ai_imc_prepare_hs_ssip(1ULL);
    goto_priv(PRIV_HS);
    CSRS(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e8_roundtrip((const uint8_t *)src1_vaddr,
        (uint8_t *)dst1_vaddr, &vstart1);
    const bool second_segment_no_fault = !excpt.triggered;
    fence_iorw_iorw();

    const bool interrupt_scalar_probe_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_SSI &&
        (ai_imc_trap_mip[0] & AI_IMC_SIP_SSIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_log_page[0] == 0x696d6373686c6700ULL &&
        (CSRR(CSR_SIP) & AI_IMC_SIP_SSIP) == 0ULL;
    const bool first_segment_image_ok =
        first_segment_no_fault &&
        vstart0 == 0ULL &&
        ai_imc_bytes_match_array(dst0_paddr, pattern0, AI_IMC_VEC_SEG2_BYTES,
            &bad_index0) &&
        read8(dst0_paddr - 1U) == 0xa5U &&
        read8(dst0_paddr + AI_IMC_VEC_SEG2_BYTES) == 0x5aU;
    const bool second_segment_image_ok =
        second_segment_no_fault &&
        vstart1 == 0ULL &&
        ai_imc_bytes_match_array(dst1_paddr, pattern1, AI_IMC_VEC_SEG2_BYTES,
            &bad_index1) &&
        ai_imc_bytes_match_array(dst0_paddr, pattern0, AI_IMC_VEC_SEG2_BYTES,
            &bad_index0) &&
        read8(dst1_paddr - 1U) == 0x3cU &&
        read8(dst1_paddr + AI_IMC_VEC_SEG2_BYTES) == 0xc3U &&
        read64(handler_probe_paddr) == handler_seed &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP | AI_IMC_SIP_SSIP);
    CSRW(CSR_VSTART, 0ULL);
    ai_imc_handler_probe_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("delegated SSIP handler performs one independent scalar load and clears source between segment epochs",
        interrupt_scalar_probe_once,
        "rounds=%lx trap_count=%lx cause=%lx sip=%lx probe=%lx final_sip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_probe_value[0],
        CSRR(CSR_SIP));
    TEST_ASSERT("first vlseg2e8/vsseg2e8 epoch keeps element-major field order before the scalar-load handler",
        first_segment_image_ok,
        "fault=%d vstart=%lx bad=%lx dst0=%x,%x,%x,%x,%x,%x,%x,%x guards=%x,%x",
        first_segment_no_fault, vstart0, bad_index0,
        read8(dst0_paddr), read8(dst0_paddr + 1U),
        read8(dst0_paddr + 2U), read8(dst0_paddr + 3U),
        read8(dst0_paddr + 4U), read8(dst0_paddr + 5U),
        read8(dst0_paddr + 6U), read8(dst0_paddr + 7U),
        read8(dst0_paddr - 1U),
        read8(dst0_paddr + AI_IMC_VEC_SEG2_BYTES));
    TEST_ASSERT("second segment epoch uses its own payload after handler scalar memory and leaves first epoch, probe, and guard intact",
        second_segment_image_ok,
        "fault=%d vstart=%lx bad1=%lx bad0=%lx dst1=%x,%x,%x,%x,%x,%x,%x,%x probe=%lx guard=%lx",
        second_segment_no_fault, vstart1, bad_index1, bad_index0,
        read8(dst1_paddr), read8(dst1_paddr + 1U),
        read8(dst1_paddr + 2U), read8(dst1_paddr + 3U),
        read8(dst1_paddr + 4U), read8(dst1_paddr + 5U),
        read8(dst1_paddr + 6U), read8(dst1_paddr + 7U),
        read64(handler_probe_paddr),
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_ssip_vector_segment_handler_scalar_probe_two_epoch_corner);
}

bool ai_interrupt_memory_m_stip_vector_cross64_split_merge_image_corner()
{
    TEST_START();

    const uintptr_t src = phys_page_base(SWITCH1) + 0xe80U;
    const uintptr_t dst_line = phys_page_base(SWITCH1) + 0xec0U;
    const uintptr_t dst = dst_line + 56U;
    const uintptr_t dump = phys_page_base(SWITCH1) + 0xf40U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xf80U;
    const uint8_t old_pattern[16] = {
        0x10U, 0x11U, 0x12U, 0x13U,
        0x14U, 0x15U, 0x16U, 0x17U,
        0x18U, 0x19U, 0x1aU, 0x1bU,
        0x1cU, 0x1dU, 0x1eU, 0x1fU
    };
    const uint8_t new_pattern[16] = {
        0xa0U, 0xa1U, 0xa2U, 0xa3U,
        0xa4U, 0xa5U, 0xa6U, 0xa7U,
        0xa8U, 0xa9U, 0xaaU, 0xabU,
        0xacU, 0xadU, 0xaeU, 0xafU
    };
    const uint64_t guard_seed = 0x696d63767673706cULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t old_vstart = 0xffffffffffffffffULL;
    uint64_t new_vstart = 0xffffffffffffffffULL;
    uint64_t old_bad_index = 0ULL;
    uint64_t new_bad_index = 0ULL;
    uint64_t dump_bad_index = 0ULL;
    uint64_t rounds = 0ULL;

    TEST_ASSERT("cross64 vector destination intentionally spans a 64-byte boundary",
        (dst_line & 63ULL) == 0ULL &&
        (dst & 63ULL) == 56ULL,
        "dst_line=%lx dst=%lx", dst_line, dst);

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 16ULL; ++i) {
        write8(src + i, old_pattern[i]);
        write8(dst + i, 0xeeU);
        write8(dump + i, 0xddU);
    }
    write8(dst - 1U, 0xa5U);
    write8(dst + 16U, 0x5aU);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    TEST_SETUP_EXCEPT();
    ai_imc_vec_e8_cross64_roundtrip((const uint8_t *)src, (uint8_t *)dst,
        (uint8_t *)dump, &old_vstart);
    const bool old_round_complete =
        !excpt.triggered &&
        old_vstart == 0ULL &&
        ai_imc_bytes_match_array(dst, old_pattern, sizeof(old_pattern),
            &old_bad_index) &&
        ai_imc_bytes_match_array(dump, old_pattern, sizeof(old_pattern),
            &dump_bad_index) &&
        read8(dst - 1U) == 0xa5U &&
        read8(dst + 16U) == 0x5aU &&
        read64(guard) == guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 16ULL; ++i) {
        write8(src + i, new_pattern[i]);
        write8(dump + i, 0xccU);
    }
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    TEST_SETUP_EXCEPT();
    ai_imc_vec_e8_cross64_roundtrip((const uint8_t *)src, (uint8_t *)dst,
        (uint8_t *)dump, &new_vstart);
    const bool new_round_replaces_entire_split_image =
        !excpt.triggered &&
        new_vstart == 0ULL &&
        ai_imc_bytes_match_array(dst, new_pattern, sizeof(new_pattern),
            &new_bad_index) &&
        ai_imc_bytes_match_array(dump, new_pattern, sizeof(new_pattern),
            &dump_bad_index) &&
        read8(dst - 1U) == 0xa5U &&
        read8(dst + 16U) == 0x5aU &&
        read64(guard) == guard_seed;

    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("first cross-64B unit-stride vector store/load round writes a complete old split image with guards preserved",
        old_round_complete,
        "old_vstart=%lx old_bad=%lx dump_bad=%lx dst0=%x dst8=%x guard=%lx",
        old_vstart, old_bad_index, dump_bad_index, read8(dst),
        read8(dst + 8U), read64(guard));
    TEST_ASSERT("STIP between cross-64B vector image generations is logged once and source is cleared",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("second cross-64B unit-stride vector round replaces the whole split image and does not inherit old or handler bytes",
        new_round_replaces_entire_split_image,
        "new_vstart=%lx new_bad=%lx dump_bad=%lx dst0=%x dst8=%x log=%lx guard=%lx",
        new_vstart, new_bad_index, dump_bad_index, read8(dst),
        read8(dst + 8U), ai_imc_handler_log_page[0], read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_cross64_split_merge_image_corner);
}

bool ai_interrupt_memory_m_stip_vector_whole_register_group_owner_corner()
{
    TEST_START();

    const uintptr_t src = (uintptr_t)&ai_imc_vec_whole_src[0];
    const uintptr_t dst = (uintptr_t)&ai_imc_vec_whole_dst[8U];
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1e80U;
    const uint64_t guard_seed = 0x696d637677686f01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t vlenb = 0ULL;
    uint64_t vl_before_store = 0xffffffffffffffffULL;
    uint64_t final_vstart = 0xffffffffffffffffULL;
    uint64_t bad_index = 0ULL;

    ai_imc_enable_vector_state();
    vlenb = ai_imc_read_vlenb();

    TEST_ASSERT("whole-register vector copy uses a VLENB that fits the static DRAM window",
        vlenb > 0ULL && 2ULL * vlenb <= AI_IMC_VEC_WHOLE_MAX_BYTES,
        "vlenb=%lx max=%x", vlenb, AI_IMC_VEC_WHOLE_MAX_BYTES);

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 2ULL * vlenb; ++i) {
        write8(src + i, (uint8_t)(0x30U + ((i * 17U) & 0x3fU)));
        write8(dst + i, 0x91U);
    }
    write8(dst - 1U, 0xa5U);
    write8(dst + 2ULL * vlenb, 0x5aU);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_whole_2r_copy((const uint8_t *)src, (uint8_t *)dst,
        &vl_before_store, &final_vstart);
    const bool no_fault = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool whole_register_ok =
        no_fault &&
        vl_before_store == 2ULL &&
        final_vstart == 0ULL &&
        ai_imc_bytes_match_array(dst, (const uint8_t *)src,
            2ULL * vlenb, &bad_index) &&
        read8(dst - 1U) == 0xa5U &&
        read8(dst + 2ULL * vlenb) == 0x5aU &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP after whole-register vector memory is logged once and does not corrupt source clear",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("vl2re8/vs2r copies exactly the two-register byte group and keeps neighboring guards after the interrupt window",
        whole_register_ok,
        "no_fault=%d vlenb=%lx vl=%lx vstart=%lx bad=%lx dst0=%x dst_last=%x guard_lo=%x guard_hi=%x marker=%lx",
        no_fault, vlenb, vl_before_store, final_vstart, bad_index,
        read8(dst), read8(dst + 2ULL * vlenb - 1ULL),
        read8(dst - 1U), read8(dst + 2ULL * vlenb), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_whole_register_group_owner_corner);
}

bool ai_interrupt_memory_m_stip_vector_zero_active_no_side_effect_corner()
{
    TEST_START();

    const uintptr_t old_src = phys_page_base(SCRATCHPAD) + 0xd80U;
    const uintptr_t new_src = phys_page_base(SCRATCHPAD) + 0xdc0U;
    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0xe00U;
    const uintptr_t dump = phys_page_base(SCRATCHPAD) + 0xe40U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xe80U;
    const uint32_t old_words[AI_IMC_VEC_VL4] = {
        0x11111111U, 0x22222222U, 0x33333333U, 0x44444444U
    };
    const uint32_t new_words[AI_IMC_VEC_VL4] = {
        0xa1a1a1a1U, 0xb2b2b2b2U, 0xc3c3c3c3U, 0xd4d4d4d4U
    };
    const uint64_t guard_seed = 0x696d63767a616301ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t final_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(old_src + i * sizeof(uint32_t), old_words[i]);
        write32(new_src + i * sizeof(uint32_t), new_words[i]);
        write32(target + i * sizeof(uint32_t), old_words[i]);
        write32(dump + i * sizeof(uint32_t), 0xeeeeeeeeU);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_zero_active_roundtrip((const uint32_t *)old_src,
        (const uint32_t *)new_src, (uint32_t *)target,
        (uint32_t *)dump, &final_vstart);
    const bool no_fault = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool zero_active_ok =
        no_fault &&
        final_vstart == 0ULL &&
        ai_imc_words_match(dump, old_words, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(target, old_words, AI_IMC_VEC_VL4) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP after zero-active vector memory is logged exactly once and clears pending state",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("zero-active vector load/store leaves destination and memory image unchanged rather than leaking LSQ side effects",
        zero_active_ok,
        "no_fault=%d vstart=%lx dump=%x,%x,%x,%x target=%x,%x,%x,%x guard=%lx",
        no_fault, final_vstart,
        read32(dump), read32(dump + 4U), read32(dump + 8U),
        read32(dump + 12U), read32(target), read32(target + 4U),
        read32(target + 8U), read32(target + 12U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_zero_active_no_side_effect_corner);
}

bool ai_interrupt_memory_hs_stip_vfof_nonzero_vl_then_fault_isolation_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const enum test_page fault_page = VSRW_GRWX;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_IMC_VFOF_PREFIX_BYTES;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_IMC_VFOF_PREFIX_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xf00U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xf00U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xf40U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xf40U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x180U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x180U;
    const uintptr_t guard = phys_page_base(aux_page) + 0xf90U;
    const uint8_t prefix[AI_IMC_VFOF_PREFIX_BYTES] = {
        0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U, 0x88U
    };
    const uint8_t blocked[AI_IMC_VFOF_PREFIX_BYTES] = {
        0x91U, 0xa2U, 0xb3U, 0xc4U, 0xd5U, 0xe6U, 0xf7U, 0x08U
    };
    const uint8_t seed[AI_IMC_VFOF_FULL_VL] = {
        0xc0U, 0xc1U, 0xc2U, 0xc3U, 0xc4U, 0xc5U, 0xc6U, 0xc7U,
        0xc8U, 0xc9U, 0xcaU, 0xcbU, 0xccU, 0xcdU, 0xceU, 0xcfU
    };
    const uint64_t fault_seed = 0x696d6376666f6601ULL;
    const uint64_t guard_seed = 0x696d6376666f66aaULL;
    const uint64_t fault_sentinel = 0x696d6376666f66ddULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t observed_vl = 0xffffffffffffffffULL;
    uint64_t observed_vstart = 0xffffffffffffffffULL;
    uint64_t fault_result = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t retry_result = 0ULL;
    uint64_t bad_index = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    ai_imc_write_bytes(low_tail_paddr, prefix, AI_IMC_VFOF_PREFIX_BYTES);
    ai_imc_write_bytes(high_head_paddr, blocked,
        AI_IMC_VFOF_PREFIX_BYTES);
    ai_imc_write_bytes(seed_paddr, seed, AI_IMC_VFOF_FULL_VL);
    for (uint64_t i = 0ULL; i < AI_IMC_VFOF_FULL_VL; ++i) {
        write8(dump_paddr + i, 0xeeU);
    }
    write64(fault_paddr, fault_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_imc_vec_vle8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)dump_vaddr,
        AI_IMC_VFOF_FULL_VL, &observed_vl, &observed_vstart);
    const bool vfof_excpt_triggered = excpt.triggered;
    const uint64_t vfof_excpt_cause = excpt.cause;
    const uint64_t vfof_excpt_tval = excpt.tval;
    const bool vfof_shortened_without_visible_trap =
        !excpt.triggered &&
        observed_vl == AI_IMC_VFOF_PREFIX_BYTES &&
        observed_vstart == 0ULL &&
        ai_imc_bytes_match_array(dump_paddr, prefix,
            AI_IMC_VFOF_PREFIX_BYTES, &bad_index) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_logged_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fault_result = ai_imc_ld_probe(fault_vaddr, fault_sentinel);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    const bool later_fault_refreshes_cause_tval =
        excpt.triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == fault_vaddr &&
        fault_tval != hs_page_base(high_page) &&
        fault_result == fault_sentinel &&
        read64(guard) == guard_seed;

    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    TEST_SETUP_EXCEPT();
    retry_result = ai_imc_ld_probe(fault_vaddr, fault_sentinel);
    const bool repaired_load_reads_original_seed =
        !excpt.triggered &&
        retry_result == fault_seed &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("nonzero-element VFOF page fault silently shortens VL, keeps vstart clear, and preserves the readable prefix payload",
        vfof_shortened_without_visible_trap,
        "triggered=%d cause=%lx tval=%lx vl=%lx vstart=%lx bad=%lx dump0=%x dump7=%x guard=%lx",
        vfof_excpt_triggered, vfof_excpt_cause, vfof_excpt_tval,
        observed_vl,
        observed_vstart, bad_index, read8(dump_paddr),
        read8(dump_paddr + 7U), read64(guard));
    TEST_ASSERT("STIP handler memory logging after the VFOF episode occurs exactly once and clears the pending source",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0]);
    TEST_ASSERT("a later real scalar load page fault refreshes cause/tval instead of reusing the silent VFOF fault address",
        later_fault_refreshes_cause_tval && repaired_load_reads_original_seed,
        "fault_cause=%lx fault_tval=%lx expected=%lx result=%lx retry=%lx guard=%lx",
        fault_cause, fault_tval, fault_vaddr, fault_result,
        retry_result, read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_vfof_nonzero_vl_then_fault_isolation_corner);
}

bool ai_interrupt_memory_hs_stip_vfof_elem0_fault_pending_retry_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH1;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(fault_page) + 0x280U;
    const uintptr_t src_paddr = phys_page_base(fault_page) + 0x280U;
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0xfa0U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0xfa0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xfc0U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xfc0U;
    const uintptr_t guard = phys_page_base(aux_page) + 0xff0U;
    const uint8_t source[AI_IMC_VFOF_FULL_VL] = {
        0x41U, 0x42U, 0x43U, 0x44U, 0x45U, 0x46U, 0x47U, 0x48U,
        0x49U, 0x4aU, 0x4bU, 0x4cU, 0x4dU, 0x4eU, 0x4fU, 0x50U
    };
    const uint8_t seed[AI_IMC_VFOF_FULL_VL] = {
        0xd0U, 0xd1U, 0xd2U, 0xd3U, 0xd4U, 0xd5U, 0xd6U, 0xd7U,
        0xd8U, 0xd9U, 0xdaU, 0xdbU, 0xdcU, 0xddU, 0xdeU, 0xdfU
    };
    const uint64_t guard_seed = 0x696d637666656c30ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_vl = 0xffffffffffffffffULL;
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t bad_index = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    ai_imc_write_bytes(src_paddr, source, AI_IMC_VFOF_FULL_VL);
    ai_imc_write_bytes(seed_paddr, seed, AI_IMC_VFOF_FULL_VL);
    for (uint64_t i = 0ULL; i < AI_IMC_VFOF_FULL_VL; ++i) {
        write8(dump_paddr + i, 0xeeU);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vle8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)dump_vaddr,
        AI_IMC_VFOF_FULL_VL, &fault_vl, &fault_vstart);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    const bool elem0_fault_is_visible =
        excpt.triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == src_vaddr &&
        fault_vstart == 0ULL &&
        fault_vl == AI_IMC_VFOF_FULL_VL &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool pending_survives_visible_fault =
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    for (uint64_t i = 0ULL; i < AI_IMC_VFOF_FULL_VL; ++i) {
        write8(dump_paddr + i, 0xeeU);
    }
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vle8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)dump_vaddr,
        AI_IMC_VFOF_FULL_VL, &retry_vl, &retry_vstart);
    const bool repaired_vfof_reads_full_source =
        !excpt.triggered &&
        retry_vl == AI_IMC_VFOF_FULL_VL &&
        retry_vstart == 0ULL &&
        ai_imc_bytes_match_array(dump_paddr, source, AI_IMC_VFOF_FULL_VL,
            &bad_index) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool preserved_interrupt_delivers_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("element0 VFOF page fault is a visible precise load page fault, not a silent shortened-VL completion",
        elem0_fault_is_visible,
        "cause=%lx tval=%lx expected=%lx vl=%lx vstart=%lx bad=%lx dump0=%x guard=%lx",
        fault_cause, fault_tval, src_vaddr, fault_vl, fault_vstart,
        bad_index, read8(dump_paddr), read64(guard));
    TEST_ASSERT("the disabled pending STIP survives the visible element0 VFOF fault until the memory fault is repaired",
        pending_survives_visible_fault,
        "pending=%lx trap_count=%lx guard=%lx",
        pending_after_fault, ai_imc_trap_count, read64(guard));
    TEST_ASSERT("after page repair, VFOF retries from a clean vstart and the preserved STIP is delivered exactly once",
        repaired_vfof_reads_full_source && preserved_interrupt_delivers_once,
        "retry_vl=%lx retry_vstart=%lx rounds=%lx trap_count=%lx cause=%lx mip=%lx bad=%lx guard=%lx",
        retry_vl, retry_vstart, interrupt_rounds, ai_imc_trap_count,
        ai_imc_trap_cause[0], ai_imc_trap_mip[0], bad_index,
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_vfof_elem0_fault_pending_retry_corner);
}

bool ai_interrupt_memory_hs_stip_vfof_shortened_tail_policy_owner_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        AI_IMC_VFOF_PREFIX_BYTES;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        AI_IMC_VFOF_PREFIX_BYTES;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x100U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x100U;
    const uintptr_t vfof_dump_vaddr = hs_page_base(aux_page) + 0x140U;
    const uintptr_t vfof_dump_paddr = phys_page_base(aux_page) + 0x140U;
    const uintptr_t old_src = phys_page_base(aux_page) + 0x180U;
    const uintptr_t new_src = phys_page_base(aux_page) + 0x1c0U;
    const uintptr_t tail_dump = phys_page_base(aux_page) + 0x200U;
    const uintptr_t guard = phys_page_base(aux_page) + 0x240U;
    const uint8_t prefix[AI_IMC_VFOF_PREFIX_BYTES] = {
        0x21U, 0x32U, 0x43U, 0x54U, 0x65U, 0x76U, 0x87U, 0x98U
    };
    const uint8_t blocked[AI_IMC_VFOF_PREFIX_BYTES] = {
        0xa1U, 0xb2U, 0xc3U, 0xd4U, 0xe5U, 0xf6U, 0x07U, 0x18U
    };
    const uint8_t seed[AI_IMC_VFOF_FULL_VL] = {
        0x80U, 0x81U, 0x82U, 0x83U, 0x84U, 0x85U, 0x86U, 0x87U,
        0x88U, 0x89U, 0x8aU, 0x8bU, 0x8cU, 0x8dU, 0x8eU, 0x8fU
    };
    const uint32_t old_words[AI_IMC_VEC_VL4] = {
        0x51515151U, 0x62626262U, 0x73737373U, 0x84848484U
    };
    const uint32_t new_words[AI_IMC_VEC_VL4] = {
        0x91919191U, 0xa2a2a2a2U, 0xb3b3b3b3U, 0xc4c4c4c4U
    };
    const uint32_t expected_tail[AI_IMC_VEC_VL4] = {
        0x91919191U, 0x62626262U, 0xb3b3b3b3U, 0x84848484U
    };
    const uint64_t guard_seed = 0x696d6376667461aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t observed_vl = 0xffffffffffffffffULL;
    uint64_t observed_vstart = 0xffffffffffffffffULL;
    uint64_t tail_vstart = 0xffffffffffffffffULL;
    uint64_t bad_index = 0ULL;

    goto_priv(PRIV_M);
    ai_imc_write_bytes(low_tail_paddr, prefix, AI_IMC_VFOF_PREFIX_BYTES);
    ai_imc_write_bytes(high_head_paddr, blocked,
        AI_IMC_VFOF_PREFIX_BYTES);
    ai_imc_write_bytes(seed_paddr, seed, AI_IMC_VFOF_FULL_VL);
    for (uint64_t i = 0ULL; i < AI_IMC_VFOF_FULL_VL; ++i) {
        write8(vfof_dump_paddr + i, 0xeeU);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(old_src + i * sizeof(uint32_t), old_words[i]);
        write32(new_src + i * sizeof(uint32_t), new_words[i]);
        write32(tail_dump + i * sizeof(uint32_t), 0xddddddddU);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_imc_vec_vle8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)vfof_dump_vaddr,
        AI_IMC_VFOF_FULL_VL, &observed_vl, &observed_vstart);
    const bool vfof_shortened_once =
        !excpt.triggered &&
        observed_vl == AI_IMC_VFOF_PREFIX_BYTES &&
        observed_vstart == 0ULL &&
        ai_imc_bytes_match_array(vfof_dump_paddr, prefix,
            AI_IMC_VFOF_PREFIX_BYTES, &bad_index) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_masked_tail_load_dump((const uint32_t *)old_src,
        (const uint32_t *)new_src, (uint32_t *)tail_dump, 3ULL,
        &tail_vstart);
    const bool tail_op_no_fault = !excpt.triggered;
    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool new_tail_policy_owner =
        tail_op_no_fault &&
        tail_vstart == 0ULL &&
        ai_imc_words_match(tail_dump, expected_tail, AI_IMC_VEC_VL4) &&
        read64(guard) == guard_seed &&
        read32(tail_dump) != (uint32_t)ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("nonzero-element VFOF shortens VL once and preserves only the readable prefix before the interrupt",
        vfof_shortened_once,
        "vl=%lx vstart=%lx bad=%lx dump0=%x dump7=%x guard=%lx",
        observed_vl, observed_vstart, bad_index,
        read8(vfof_dump_paddr), read8(vfof_dump_paddr + 7U),
        read64(guard));
    TEST_ASSERT("STIP after the shortened-VL VFOF episode is logged exactly once and clears source state",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx handler_log=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("the next masked tail vector load uses the new mask and tail policy rather than stale VFOF suffix or handler data",
        new_tail_policy_owner,
        "tail_vstart=%lx tail=%x,%x,%x,%x expected=%x,%x,%x,%x guard=%lx",
        tail_vstart, read32(tail_dump), read32(tail_dump + 4U),
        read32(tail_dump + 8U), read32(tail_dump + 12U),
        expected_tail[0], expected_tail[1], expected_tail[2],
        expected_tail[3], read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_vfof_shortened_tail_policy_owner_corner);
}

bool ai_interrupt_memory_hs_stip_segment_fof_fixvl_retry_owner_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 6U;
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        6U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t seed_vaddr = hs_page_base(aux_page) + 0x280U;
    const uintptr_t seed_paddr = phys_page_base(aux_page) + 0x280U;
    const uintptr_t first_dump_vaddr = hs_page_base(aux_page) + 0x2c0U;
    const uintptr_t first_dump_paddr = phys_page_base(aux_page) + 0x2c0U;
    const uintptr_t retry_dump_vaddr = hs_page_base(aux_page) + 0x300U;
    const uintptr_t retry_dump_paddr = phys_page_base(aux_page) + 0x300U;
    const uintptr_t guard = phys_page_base(aux_page) + 0x340U;
    const uint8_t first_source[AI_IMC_VEC_SEG2_BYTES] = {
        0x31U, 0xa1U, 0x42U, 0xb2U,
        0x53U, 0xc3U, 0x64U, 0xd4U
    };
    const uint8_t retry_source[AI_IMC_VEC_SEG2_BYTES] = {
        0x75U, 0xe5U, 0x86U, 0xf6U,
        0x97U, 0x17U, 0xa8U, 0x28U
    };
    const uint8_t seed[AI_IMC_VEC_SEG2_BYTES] = {
        0xc0U, 0xc1U, 0xc2U, 0xc3U,
        0xc4U, 0xc5U, 0xc6U, 0xc7U
    };
    const uint64_t guard_seed = 0x696d6373666f66aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t first_vl = 0xffffffffffffffffULL;
    uint64_t first_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t first_bad_index = 0ULL;
    uint64_t retry_bad_index = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 6ULL; ++i) {
        write8(low_tail_paddr + i, first_source[i]);
    }
    write8(high_head_paddr, first_source[6]);
    write8(high_head_paddr + 1U, first_source[7]);
    ai_imc_write_bytes(seed_paddr, seed, AI_IMC_VEC_SEG2_BYTES);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_SEG2_BYTES; ++i) {
        write8(first_dump_paddr + i, 0xeeU);
        write8(retry_dump_paddr + i, 0xddU);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    ai_imc_vec_vlseg2e8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)first_dump_vaddr,
        AI_IMC_VEC_SEG2_BYTES / 2U, &first_vl, &first_vstart);
    const bool first_segment_fof_shortened_once =
        !excpt.triggered &&
        first_vl == 3ULL &&
        first_vstart == 0ULL &&
        ai_imc_bytes_match_array(first_dump_paddr, first_source, 6ULL,
            &first_bad_index) &&
        read8(first_dump_paddr + 6U) == seed[6] &&
        read8(first_dump_paddr + 7U) == seed[7] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        read64(guard) == guard_seed;

    for (uint64_t i = 0ULL; i < 6ULL; ++i) {
        write8(low_tail_paddr + i, retry_source[i]);
    }
    write8(high_head_paddr, retry_source[6]);
    write8(high_head_paddr + 1U, retry_source[7]);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vlseg2e8ff_capture((const uint8_t *)seed_vaddr,
        (const uint8_t *)src_vaddr, (uint8_t *)retry_dump_vaddr,
        AI_IMC_VEC_SEG2_BYTES / 2U, &retry_vl, &retry_vstart);
    const bool retry_segment_fof_uses_new_episode =
        !excpt.triggered &&
        retry_vl == AI_IMC_VEC_SEG2_BYTES / 2U &&
        retry_vstart == 0ULL &&
        ai_imc_bytes_match_array(retry_dump_paddr, retry_source,
            AI_IMC_VEC_SEG2_BYTES, &retry_bad_index) &&
        !ai_imc_bytes_match(retry_dump_paddr, first_source,
            AI_IMC_VEC_SEG2_BYTES) &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("segment FOF first fault in the last field shortens VL to the last complete element and leaves the faulting element undisturbed",
        first_segment_fof_shortened_once,
        "vl=%lx vstart=%lx bad=%lx dump=%x,%x,%x,%x,%x,%x,%x,%x guard=%lx",
        first_vl, first_vstart, first_bad_index,
        read8(first_dump_paddr), read8(first_dump_paddr + 1U),
        read8(first_dump_paddr + 2U), read8(first_dump_paddr + 3U),
        read8(first_dump_paddr + 4U), read8(first_dump_paddr + 5U),
        read8(first_dump_paddr + 6U), read8(first_dump_paddr + 7U),
        read64(guard));
    TEST_ASSERT("STIP between shortened segment-FOF and retry is logged exactly once and clears source state",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("after page repair the next segment FOF uses a fresh full-VL source instead of stale shortened-VL or handler metadata",
        retry_segment_fof_uses_new_episode,
        "vl=%lx vstart=%lx bad=%lx retry=%x,%x,%x,%x,%x,%x,%x,%x guard=%lx",
        retry_vl, retry_vstart, retry_bad_index,
        read8(retry_dump_paddr), read8(retry_dump_paddr + 1U),
        read8(retry_dump_paddr + 2U), read8(retry_dump_paddr + 3U),
        read8(retry_dump_paddr + 4U), read8(retry_dump_paddr + 5U),
        read8(retry_dump_paddr + 6U), read8(retry_dump_paddr + 7U),
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_segment_fof_fixvl_retry_owner_corner);
}

bool ai_interrupt_memory_hs_stip_vector_store_fault_repair_vstart_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_tail_paddr = phys_page_base(low_page) +
        PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t src_vaddr = hs_page_base(src_page) + 0x680U;
    const uintptr_t src_paddr = phys_page_base(src_page) + 0x680U;
    const uintptr_t retry_src_vaddr = hs_page_base(src_page) + 0x6c0U;
    const uintptr_t retry_src_paddr = phys_page_base(src_page) + 0x6c0U;
    const uintptr_t low_guard_paddr = low_tail_paddr - sizeof(uint64_t);
    const uintptr_t high_guard_paddr = high_head_paddr + 8U;
    const uint32_t old_words[AI_IMC_VEC_VL4] = {
        0x51010000U, 0x51010001U, 0x51010002U, 0x51010003U
    };
    const uint32_t src_words[AI_IMC_VEC_VL4] = {
        0x76040010U, 0x76040011U, 0x76040012U, 0x76040013U
    };
    const uint32_t retry_words[AI_IMC_VEC_VL4] = {
        0xbad00020U, 0xbad00021U, 0x76040012U, 0x76040013U
    };
    const uint64_t low_guard_seed = 0x696d637673727001ULL;
    const uint64_t high_guard_seed = 0x696d637673727002ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t fault_vl = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t vstart_after_interrupt = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    write32(low_tail_paddr + 0U, old_words[0]);
    write32(low_tail_paddr + 4U, old_words[1]);
    write32(high_head_paddr + 0U, old_words[2]);
    write32(high_head_paddr + 4U, old_words[3]);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(src_paddr + i * sizeof(uint32_t), src_words[i]);
        write32(retry_src_paddr + i * sizeof(uint32_t), retry_words[i]);
    }
    write64(low_guard_paddr, low_guard_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src_vaddr, AI_IMC_VEC_VL4);
    const bool source_stage_no_fault = !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        CSRR(CSR_VL) == AI_IMC_VEC_VL4;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst_vaddr,
        0ULL, &fault_vl, &fault_vstart);
    const bool fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool precise_fault_contract =
        source_stage_no_fault &&
        fault_triggered &&
        fault_cause == CAUSE_SPF &&
        fault_tval == fault_elem_vaddr &&
        fault_vstart == 2ULL &&
        fault_vl == AI_IMC_VEC_VL4 &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;
    const bool prefix_only_after_fault =
        read32(low_tail_paddr + 0U) == src_words[0] &&
        read32(low_tail_paddr + 4U) == src_words[1] &&
        read32(high_head_paddr + 0U) == old_words[2] &&
        read32(high_head_paddr + 4U) == old_words[3] &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    vstart_after_interrupt = CSRR(CSR_VSTART);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_log_preserves_fault_state =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        vstart_after_interrupt == fault_vstart &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)retry_src_vaddr, AI_IMC_VEC_VL4);
    const bool retry_source_stage_no_fault = !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        CSRR(CSR_VL) == AI_IMC_VEC_VL4;

    CSRW(CSR_VSTART, fault_vstart);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst_vaddr,
        fault_vstart, &retry_vl, &retry_vstart);
    const bool retry_no_fault = retry_source_stage_no_fault &&
        !excpt.triggered &&
        retry_vstart == 0ULL &&
        retry_vl == AI_IMC_VEC_VL4;

    goto_priv(PRIV_M);
    const bool final_image_uses_captured_vstart =
        read32(low_tail_paddr + 0U) == src_words[0] &&
        read32(low_tail_paddr + 4U) == src_words[1] &&
        read32(low_tail_paddr + 0U) != retry_words[0] &&
        read32(low_tail_paddr + 4U) != retry_words[1] &&
        read32(high_head_paddr + 0U) == src_words[2] &&
        read32(high_head_paddr + 4U) == src_words[3] &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;
    const bool source_images_intact =
        ai_imc_words_match(src_paddr, src_words, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(retry_src_paddr, retry_words,
            AI_IMC_VEC_VL4);

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("unit-stride vector store fault reports element2 with precise tval/vstart while disabled STIP remains pending",
        precise_fault_contract,
        "stage_ok=%d cause=%lx tval=%lx expected=%lx vstart=%lx vl=%lx pending=%lx traps=%lx",
        source_stage_no_fault, fault_cause, fault_tval, fault_elem_vaddr,
        fault_vstart, fault_vl, pending_after_fault,
        ai_imc_trap_count);
    TEST_ASSERT("before repair, only the prefix elements are visible and the fault page plus guards keep the old image",
        prefix_only_after_fault,
        "low=%x,%x high=%x,%x guards=%lx,%lx",
        read32(low_tail_paddr), read32(low_tail_paddr + 4U),
        read32(high_head_paddr), read32(high_head_paddr + 4U),
        read64(low_guard_paddr), read64(high_guard_paddr));
    TEST_ASSERT("the preserved STIP is delivered exactly once before repair and the handler does not clobber the captured vector vstart",
        interrupt_log_preserves_fault_state,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx vstart_after_int=%lx fault_vstart=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], vstart_after_interrupt, fault_vstart,
        CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the high-page leaf, retry from captured vstart writes only suffix elements and does not replay poisoned prefix lanes",
        retry_no_fault && final_image_uses_captured_vstart &&
            source_images_intact,
        "retry_stage=%d retry_vstart=%lx retry_vl=%lx low=%x,%x high=%x,%x src_ok=%d",
        retry_source_stage_no_fault, retry_vstart, retry_vl,
        read32(low_tail_paddr), read32(low_tail_paddr + 4U),
        read32(high_head_paddr), read32(high_head_paddr + 4U),
        source_images_intact);

    TEST_END(ai_interrupt_memory_hs_stip_vector_store_fault_repair_vstart_corner);
}

bool ai_interrupt_memory_hs_stip_vector_store_two_late_faults_suffix_isolation_corner()
{
    TEST_START();

    const enum test_page low0_page = VSRWX_GRWX;
    const enum test_page high0_page = VSRWX_GRW;
    const enum test_page low1_page = VSRW_GRWX;
    const enum test_page high1_page = VSRW_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst0_vaddr = hs_page_base(low0_page) + PAGE_SIZE - 8U;
    const uintptr_t dst1_vaddr = hs_page_base(low1_page) + PAGE_SIZE - 8U;
    const uintptr_t fault0_vaddr = hs_page_base(high0_page);
    const uintptr_t fault1_vaddr = hs_page_base(high1_page);
    const uintptr_t low0_paddr = phys_page_base(low0_page) + PAGE_SIZE - 8U;
    const uintptr_t high0_paddr = phys_page_base(high0_page);
    const uintptr_t low1_paddr = phys_page_base(low1_page) + PAGE_SIZE - 8U;
    const uintptr_t high1_paddr = phys_page_base(high1_page);
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x800U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x840U;
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x800U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x840U;
    const uintptr_t low0_guard = low0_paddr - sizeof(uint64_t);
    const uintptr_t high0_guard = high0_paddr + 2U * sizeof(uint32_t);
    const uintptr_t low1_guard = low1_paddr - sizeof(uint64_t);
    const uintptr_t high1_guard = high1_paddr + 2U * sizeof(uint32_t);
    const uint32_t old0[AI_IMC_VEC_VL4] = {
        0x71000000U, 0x71000001U, 0x71000002U, 0x71000003U
    };
    const uint32_t old1[AI_IMC_VEC_VL4] = {
        0x72000000U, 0x72000001U, 0x72000002U, 0x72000003U
    };
    const uint32_t src0_words[AI_IMC_VEC_VL4] = {
        0x71330000U, 0x71330001U, 0x71330002U, 0x71330003U
    };
    const uint32_t src1_words[AI_IMC_VEC_VL4] = {
        0x72440000U, 0x72440001U, 0x72440002U, 0x72440003U
    };
    const uint64_t low0_guard_seed = 0x696d637336373031ULL;
    const uint64_t high0_guard_seed = 0x696d637336373032ULL;
    const uint64_t low1_guard_seed = 0x696d637336373033ULL;
    const uint64_t high1_guard_seed = 0x696d637336373034ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault0_vstart = 0xffffffffffffffffULL;
    uint64_t fault0_vl = 0xffffffffffffffffULL;
    uint64_t retry0_vstart = 0xffffffffffffffffULL;
    uint64_t retry0_vl = 0xffffffffffffffffULL;
    uint64_t fault1_vstart = 0xffffffffffffffffULL;
    uint64_t fault1_vl = 0xffffffffffffffffULL;
    uint64_t retry1_vstart = 0xffffffffffffffffULL;
    uint64_t retry1_vl = 0xffffffffffffffffULL;
    uint64_t fault0_cause = 0ULL;
    uint64_t fault0_tval = 0ULL;
    uint64_t fault1_cause = 0ULL;
    uint64_t fault1_tval = 0ULL;
    uint64_t pending_after_fault0 = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(src0_paddr + i * sizeof(uint32_t), src0_words[i]);
        write32(src1_paddr + i * sizeof(uint32_t), src1_words[i]);
    }
    write32(low0_paddr + 0U, old0[0]);
    write32(low0_paddr + 4U, old0[1]);
    write32(high0_paddr + 0U, old0[2]);
    write32(high0_paddr + 4U, old0[3]);
    write32(low1_paddr + 0U, old1[0]);
    write32(low1_paddr + 4U, old1[1]);
    write32(high1_paddr + 0U, old1[2]);
    write32(high1_paddr + 4U, old1[3]);
    write64(low0_guard, low0_guard_seed);
    write64(high0_guard, high0_guard_seed);
    write64(low1_guard, low1_guard_seed);
    write64(high1_guard, high1_guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low0_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high0_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(low1_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high1_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src0_vaddr, AI_IMC_VEC_VL4);
    const bool source0_loaded =
        !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        CSRR(CSR_VL) == AI_IMC_VEC_VL4;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst0_vaddr,
        0ULL, &fault0_vl, &fault0_vstart);
    fault0_cause = excpt.cause;
    fault0_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault0 = CSRR(CSR_MIP);
    const bool first_fault_contract =
        source0_loaded &&
        excpt.triggered &&
        fault0_cause == CAUSE_SPF &&
        fault0_tval == fault0_vaddr &&
        fault0_vstart == 2ULL &&
        fault0_vl == AI_IMC_VEC_VL4 &&
        (pending_after_fault0 & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;
    const bool first_prefix_only =
        read32(low0_paddr + 0U) == src0_words[0] &&
        read32(low0_paddr + 4U) == src0_words[1] &&
        read32(high0_paddr + 0U) == old0[2] &&
        read32(high0_paddr + 4U) == old0[3] &&
        read32(low1_paddr + 0U) == old1[0] &&
        read32(low1_paddr + 4U) == old1[1] &&
        read32(high1_paddr + 0U) == old1[2] &&
        read32(high1_paddr + 4U) == old1[3] &&
        read64(low0_guard) == low0_guard_seed &&
        read64(high0_guard) == high0_guard_seed &&
        read64(low1_guard) == low1_guard_seed &&
        read64(high1_guard) == high1_guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_between_faults =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        read32(high0_paddr + 0U) == old0[2] &&
        read32(high1_paddr + 0U) == old1[2];

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high0_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_VSTART, fault0_vstart);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst0_vaddr,
        fault0_vstart, &retry0_vl, &retry0_vstart);
    const bool first_retry_complete =
        !excpt.triggered &&
        retry0_vstart == 0ULL &&
        retry0_vl == AI_IMC_VEC_VL4 &&
        read32(low0_paddr + 0U) == src0_words[0] &&
        read32(low0_paddr + 4U) == src0_words[1] &&
        read32(high0_paddr + 0U) == src0_words[2] &&
        read32(high0_paddr + 4U) == src0_words[3];

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)src1_vaddr, AI_IMC_VEC_VL4);
    const bool source1_loaded =
        !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        CSRR(CSR_VL) == AI_IMC_VEC_VL4;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst1_vaddr,
        0ULL, &fault1_vl, &fault1_vstart);
    fault1_cause = excpt.cause;
    fault1_tval = excpt.tval;

    goto_priv(PRIV_M);
    const bool second_fault_contract =
        source1_loaded &&
        excpt.triggered &&
        fault1_cause == CAUSE_SPF &&
        fault1_tval == fault1_vaddr &&
        fault1_vstart == 2ULL &&
        fault1_vl == AI_IMC_VEC_VL4;
    const bool second_fault_uses_new_suffix_state =
        read32(low1_paddr + 0U) == src1_words[0] &&
        read32(low1_paddr + 4U) == src1_words[1] &&
        read32(high1_paddr + 0U) == old1[2] &&
        read32(high1_paddr + 4U) == old1[3] &&
        read32(high1_paddr + 0U) != src0_words[2] &&
        read32(high1_paddr + 4U) != src0_words[3] &&
        read32(low0_paddr + 0U) == src0_words[0] &&
        read32(high0_paddr + 4U) == src0_words[3] &&
        read64(low0_guard) == low0_guard_seed &&
        read64(high0_guard) == high0_guard_seed &&
        read64(low1_guard) == low1_guard_seed &&
        read64(high1_guard) == high1_guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high1_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_VSTART, fault1_vstart);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)dst1_vaddr,
        fault1_vstart, &retry1_vl, &retry1_vstart);
    const bool second_retry_complete =
        !excpt.triggered &&
        retry1_vstart == 0ULL &&
        retry1_vl == AI_IMC_VEC_VL4;

    goto_priv(PRIV_M);
    const bool final_images_separate =
        read32(low0_paddr + 0U) == src0_words[0] &&
        read32(low0_paddr + 4U) == src0_words[1] &&
        read32(high0_paddr + 0U) == src0_words[2] &&
        read32(high0_paddr + 4U) == src0_words[3] &&
        read32(low1_paddr + 0U) == src1_words[0] &&
        read32(low1_paddr + 4U) == src1_words[1] &&
        read32(high1_paddr + 0U) == src1_words[2] &&
        read32(high1_paddr + 4U) == src1_words[3] &&
        ai_imc_words_match(src0_paddr, src0_words, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(src1_paddr, src1_words, AI_IMC_VEC_VL4) &&
        read64(low0_guard) == low0_guard_seed &&
        read64(high0_guard) == high0_guard_seed &&
        read64(low1_guard) == low1_guard_seed &&
        read64(high1_guard) == high1_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the first late vector-store fault captures its own prefix, tval and vstart while the second target remains untouched",
        first_fault_contract && first_prefix_only,
        "stage=%d cause=%lx tval=%lx expected=%lx vstart=%lx vl=%lx pending=%lx low0=%x,%x high0=%x,%x low1=%x,%x",
        source0_loaded, fault0_cause, fault0_tval, fault0_vaddr,
        fault0_vstart, fault0_vl, pending_after_fault0,
        read32(low0_paddr), read32(low0_paddr + 4U),
        read32(high0_paddr), read32(high0_paddr + 4U),
        read32(low1_paddr), read32(low1_paddr + 4U));
    TEST_ASSERT("STIP is delivered exactly once between the two vector-store fault episodes and does not commit either faulting suffix early",
        interrupt_between_faults,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx high0=%x high1=%x",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP), read32(high0_paddr),
        read32(high1_paddr));
    TEST_ASSERT("the second vector-store fault refreshes cause/tval/vstart and keeps its suffix old image instead of inheriting the first episode suffix",
        first_retry_complete && second_fault_contract &&
            second_fault_uses_new_suffix_state,
        "retry0=%lx/%lx stage1=%d cause1=%lx tval1=%lx expected1=%lx vstart1=%lx vl1=%lx high1=%x,%x first_suffix=%x,%x",
        retry0_vstart, retry0_vl, source1_loaded, fault1_cause,
        fault1_tval, fault1_vaddr, fault1_vstart, fault1_vl,
        read32(high1_paddr), read32(high1_paddr + 4U),
        src0_words[2], src0_words[3]);
    TEST_ASSERT("after both repairs, each vector store owns only its own prefix and suffix payloads and all guards/sources remain intact",
        second_retry_complete && final_images_separate,
        "retry1=%lx/%lx first=%x,%x,%x,%x second=%x,%x,%x,%x guards=%lx,%lx,%lx,%lx",
        retry1_vstart, retry1_vl, read32(low0_paddr),
        read32(low0_paddr + 4U), read32(high0_paddr),
        read32(high0_paddr + 4U), read32(low1_paddr),
        read32(low1_paddr + 4U), read32(high1_paddr),
        read32(high1_paddr + 4U), read64(low0_guard),
        read64(high0_guard), read64(low1_guard), read64(high1_guard));

    TEST_END(ai_interrupt_memory_hs_stip_vector_store_two_late_faults_suffix_isolation_corner);
}

bool ai_interrupt_memory_hs_stip_vector_segment_store_fault_retry_vstart_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page src_page = SWITCH1;
    const uintptr_t dst_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_elem_vaddr = hs_page_base(high_page);
    const uintptr_t low_tail_paddr = phys_page_base(low_page) +
        PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t src0_paddr = phys_page_base(src_page) + 0x740U;
    const uintptr_t src1_paddr = phys_page_base(src_page) + 0x780U;
    const uintptr_t src0_vaddr = hs_page_base(src_page) + 0x740U;
    const uintptr_t src1_vaddr = hs_page_base(src_page) + 0x780U;
    const uintptr_t low_guard_paddr = low_tail_paddr - sizeof(uint64_t);
    const uintptr_t high_guard_paddr = high_head_paddr + 24U;
    const uint32_t old_fields[AI_IMC_VEC_VL4 * 2U] = {
        0x4f000000U, 0x4f000001U, 0x4f000002U, 0x4f000003U,
        0x4f000004U, 0x4f000005U, 0x4f000006U, 0x4f000007U
    };
    const uint32_t field0[AI_IMC_VEC_VL4] = {
        0x51000000U, 0x51000001U, 0x51000002U, 0x51000003U
    };
    const uint32_t field1[AI_IMC_VEC_VL4] = {
        0x52000000U, 0x52000001U, 0x52000002U, 0x52000003U
    };
    const uint64_t low_guard_seed = 0x696d637673656630ULL;
    const uint64_t high_guard_seed = 0x696d637673656631ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t fault_vl = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(src0_paddr + i * sizeof(uint32_t), field0[i]);
        write32(src1_paddr + i * sizeof(uint32_t), field1[i]);
    }
    write32(low_tail_paddr + 0U, old_fields[0]);
    write32(low_tail_paddr + 4U, old_fields[1]);
    for (uint64_t i = 0ULL; i < 6ULL; ++i) {
        write32(high_head_paddr + i * sizeof(uint32_t),
            old_fields[i + 2ULL]);
    }
    write64(low_guard_paddr, low_guard_seed);
    write64(high_guard_paddr, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(src_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_store((const uint32_t *)src0_vaddr,
        (const uint32_t *)src1_vaddr, (uint32_t *)dst_vaddr, 0ULL,
        &fault_vstart, &fault_vl);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool segment_fault_precise =
        excpt.triggered &&
        fault_cause == CAUSE_SPF &&
        fault_tval == fault_elem_vaddr &&
        fault_vstart == 1ULL &&
        fault_vl == AI_IMC_VEC_VL4 &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;
    const bool segment_prefix_only =
        read32(low_tail_paddr + 0U) == field0[0] &&
        read32(low_tail_paddr + 4U) == field1[0] &&
        read32(high_head_paddr + 0U) == old_fields[2] &&
        read32(high_head_paddr + 4U) == old_fields[3] &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_delivered_before_retry =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_VSTART, fault_vstart);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_store((const uint32_t *)src0_vaddr,
        (const uint32_t *)src1_vaddr, (uint32_t *)dst_vaddr,
        fault_vstart, &retry_vstart, &retry_vl);
    const bool retry_no_fault =
        !excpt.triggered &&
        retry_vstart == 0ULL &&
        retry_vl == AI_IMC_VEC_VL4;

    goto_priv(PRIV_M);
    const bool segment_final_image =
        read32(low_tail_paddr + 0U) == field0[0] &&
        read32(low_tail_paddr + 4U) == field1[0] &&
        read32(high_head_paddr + 0U) == field0[1] &&
        read32(high_head_paddr + 4U) == field1[1] &&
        read32(high_head_paddr + 8U) == field0[2] &&
        read32(high_head_paddr + 12U) == field1[2] &&
        read32(high_head_paddr + 16U) == field0[3] &&
        read32(high_head_paddr + 20U) == field1[3] &&
        read64(low_guard_paddr) == low_guard_seed &&
        read64(high_guard_paddr) == high_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("vsseg2e32 store crossing a no-write page reports the first faulting element with precise tval/vstart while STIP remains pending",
        segment_fault_precise,
        "cause=%lx tval=%lx expected=%lx vstart=%lx vl=%lx pending=%lx traps=%lx",
        fault_cause, fault_tval, fault_elem_vaddr, fault_vstart,
        fault_vl, pending_after_fault, ai_imc_trap_count);
    TEST_ASSERT("before repair, only element0 field0/field1 stores are visible and the fault page plus guards keep the old image",
        segment_prefix_only,
        "low=%x,%x high0=%x high1=%x guards=%lx,%lx",
        read32(low_tail_paddr), read32(low_tail_paddr + 4U),
        read32(high_head_paddr), read32(high_head_paddr + 4U),
        read64(low_guard_paddr), read64(high_guard_paddr));
    TEST_ASSERT("the pending STIP is delivered once between segment fault capture and retry without clobbering segment memory",
        interrupt_delivered_before_retry,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after page repair, retry from captured vstart writes only the suffix segment fields and keeps field order",
        retry_no_fault && segment_final_image,
        "retry_vstart=%lx retry_vl=%lx low=%x,%x high=%x,%x,%x,%x,%x,%x guards=%lx,%lx",
        retry_vstart, retry_vl, read32(low_tail_paddr),
        read32(low_tail_paddr + 4U), read32(high_head_paddr),
        read32(high_head_paddr + 4U), read32(high_head_paddr + 8U),
        read32(high_head_paddr + 12U), read32(high_head_paddr + 16U),
        read32(high_head_paddr + 20U), read64(low_guard_paddr),
        read64(high_guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_vector_segment_store_fault_retry_vstart_corner);
}

bool ai_interrupt_memory_hs_stip_vsegment_vsmerge_feedback_source_split_corner()
{
    TEST_START();

    const enum test_page vec_low_page = VSRWX_GRWX;
    const enum test_page vec_high_page = VSRWX_GRW;
    const enum test_page seg_dst_page = SCRATCHPAD;
    const enum test_page seg_src_page = SWITCH1;
    const enum test_page vec_src_page = SWITCH2;
    const uintptr_t seg_dst_vaddr = hs_page_base(seg_dst_page) + 0x500U;
    const uintptr_t seg_dst_paddr = phys_page_base(seg_dst_page) + 0x500U;
    const uintptr_t seg_src0_vaddr = hs_page_base(seg_src_page) + 0x700U;
    const uintptr_t seg_src1_vaddr = hs_page_base(seg_src_page) + 0x740U;
    const uintptr_t seg_src0_paddr = phys_page_base(seg_src_page) + 0x700U;
    const uintptr_t seg_src1_paddr = phys_page_base(seg_src_page) + 0x740U;
    const uintptr_t vec_src_vaddr = hs_page_base(vec_src_page) + 0x700U;
    const uintptr_t vec_src_paddr = phys_page_base(vec_src_page) + 0x700U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_low_page) +
        PAGE_SIZE - 8U;
    const uintptr_t vec_fault_vaddr = hs_page_base(vec_high_page);
    const uintptr_t vec_low_tail_paddr = phys_page_base(vec_low_page) +
        PAGE_SIZE - 8U;
    const uintptr_t vec_high_head_paddr =
        phys_page_base(vec_high_page);
    const uintptr_t seg_guard0_paddr = seg_dst_paddr - sizeof(uint64_t);
    const uintptr_t seg_guard1_paddr = seg_dst_paddr +
        AI_IMC_VEC_VL4 * 2U * sizeof(uint32_t);
    const uintptr_t vec_low_guard_paddr = vec_low_tail_paddr -
        sizeof(uint64_t);
    const uintptr_t vec_high_guard_paddr = vec_high_head_paddr +
        2U * sizeof(uint32_t);
    const uint32_t seg_old[AI_IMC_VEC_VL4 * 2U] = {
        0x68000000U, 0x68000001U, 0x68000002U, 0x68000003U,
        0x68000004U, 0x68000005U, 0x68000006U, 0x68000007U
    };
    const uint32_t seg_field0[AI_IMC_VEC_VL4] = {
        0x68110000U, 0x68110001U, 0x68110002U, 0x68110003U
    };
    const uint32_t seg_field1[AI_IMC_VEC_VL4] = {
        0x68220000U, 0x68220001U, 0x68220002U, 0x68220003U
    };
    const uint32_t seg_expected[AI_IMC_VEC_VL4 * 2U] = {
        0x68110000U, 0x68220000U, 0x68110001U, 0x68220001U,
        0x68110002U, 0x68220002U, 0x68110003U, 0x68220003U
    };
    const uint32_t vec_old[AI_IMC_VEC_VL4] = {
        0x68330000U, 0x68330001U, 0x68330002U, 0x68330003U
    };
    const uint32_t vec_words[AI_IMC_VEC_VL4] = {
        0x68440000U, 0x68440001U, 0x68440002U, 0x68440003U
    };
    const uint64_t seg_guard0_seed = 0x696d637336386730ULL;
    const uint64_t seg_guard1_seed = 0x696d637336386731ULL;
    const uint64_t vec_guard0_seed = 0x696d637336386732ULL;
    const uint64_t vec_guard1_seed = 0x696d637336386733ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t seg_vstart = 0xffffffffffffffffULL;
    uint64_t seg_vl = 0xffffffffffffffffULL;
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t fault_vl = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_segment = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t vstart_after_interrupt = 0xffffffffffffffffULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(seg_src0_paddr + i * sizeof(uint32_t),
            seg_field0[i]);
        write32(seg_src1_paddr + i * sizeof(uint32_t),
            seg_field1[i]);
        write32(vec_src_paddr + i * sizeof(uint32_t), vec_words[i]);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4 * 2ULL; ++i) {
        write32(seg_dst_paddr + i * sizeof(uint32_t), seg_old[i]);
    }
    write32(vec_low_tail_paddr + 0U, vec_old[0]);
    write32(vec_low_tail_paddr + 4U, vec_old[1]);
    write32(vec_high_head_paddr + 0U, vec_old[2]);
    write32(vec_high_head_paddr + 4U, vec_old[3]);
    write64(seg_guard0_paddr, seg_guard0_seed);
    write64(seg_guard1_paddr, seg_guard1_seed);
    write64(vec_low_guard_paddr, vec_guard0_seed);
    write64(vec_high_guard_paddr, vec_guard1_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(vec_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(vec_high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(seg_dst_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(seg_src_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(vec_src_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_store((const uint32_t *)seg_src0_vaddr,
        (const uint32_t *)seg_src1_vaddr, (uint32_t *)seg_dst_vaddr,
        0ULL, &seg_vstart, &seg_vl);
    const bool segment_clean_complete =
        !excpt.triggered &&
        seg_vstart == 0ULL &&
        seg_vl == AI_IMC_VEC_VL4;

    goto_priv(PRIV_M);
    pending_after_segment = CSRR(CSR_MIP);
    const bool segment_image_isolated =
        ai_imc_words_match(seg_dst_paddr, seg_expected,
            AI_IMC_VEC_VL4 * 2ULL) &&
        ai_imc_words_match(seg_src0_paddr, seg_field0,
            AI_IMC_VEC_VL4) &&
        ai_imc_words_match(seg_src1_paddr, seg_field1,
            AI_IMC_VEC_VL4) &&
        read64(seg_guard0_paddr) == seg_guard0_seed &&
        read64(seg_guard1_paddr) == seg_guard1_seed &&
        (pending_after_segment & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)vec_src_vaddr, AI_IMC_VEC_VL4);
    const bool vector_source_loaded =
        !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        CSRR(CSR_VL) == AI_IMC_VEC_VL4;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)vec_dst_vaddr,
        0ULL, &fault_vl, &fault_vstart);
    const bool vector_fault_triggered = excpt.triggered;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool ordinary_vs_fault_precise =
        vector_source_loaded &&
        vector_fault_triggered &&
        fault_cause == CAUSE_SPF &&
        fault_tval == vec_fault_vaddr &&
        fault_vstart == 2ULL &&
        fault_vl == AI_IMC_VEC_VL4 &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;
    const bool sources_still_split_after_fault =
        ai_imc_words_match(seg_dst_paddr, seg_expected,
            AI_IMC_VEC_VL4 * 2ULL) &&
        read32(vec_low_tail_paddr + 0U) == vec_words[0] &&
        read32(vec_low_tail_paddr + 4U) == vec_words[1] &&
        read32(vec_high_head_paddr + 0U) == vec_old[2] &&
        read32(vec_high_head_paddr + 4U) == vec_old[3] &&
        read64(seg_guard0_paddr) == seg_guard0_seed &&
        read64(seg_guard1_paddr) == seg_guard1_seed &&
        read64(vec_low_guard_paddr) == vec_guard0_seed &&
        read64(vec_high_guard_paddr) == vec_guard1_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    vstart_after_interrupt = CSRR(CSR_VSTART);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_logged_without_source_mix =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        vstart_after_interrupt == fault_vstart &&
        ai_imc_words_match(seg_dst_paddr, seg_expected,
            AI_IMC_VEC_VL4 * 2ULL) &&
        read32(vec_high_head_paddr + 0U) == vec_old[2] &&
        read32(vec_high_head_paddr + 4U) == vec_old[3];

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(vec_high_page, PTE_V | PTE_AD | PTE_RWX);
    CSRW(CSR_VSTART, fault_vstart);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_vse32_v6_vl4_from_vstart((uint32_t *)vec_dst_vaddr,
        fault_vstart, &retry_vl, &retry_vstart);
    const bool retry_no_fault =
        !excpt.triggered &&
        retry_vstart == 0ULL &&
        retry_vl == AI_IMC_VEC_VL4;

    goto_priv(PRIV_M);
    const bool final_images_remain_source_split =
        ai_imc_words_match(seg_dst_paddr, seg_expected,
            AI_IMC_VEC_VL4 * 2ULL) &&
        read32(vec_low_tail_paddr + 0U) == vec_words[0] &&
        read32(vec_low_tail_paddr + 4U) == vec_words[1] &&
        read32(vec_high_head_paddr + 0U) == vec_words[2] &&
        read32(vec_high_head_paddr + 4U) == vec_words[3] &&
        read64(seg_guard0_paddr) == seg_guard0_seed &&
        read64(seg_guard1_paddr) == seg_guard1_seed &&
        read64(vec_low_guard_paddr) == vec_guard0_seed &&
        read64(vec_high_guard_paddr) == vec_guard1_seed;
    const bool all_sources_intact =
        ai_imc_words_match(seg_src0_paddr, seg_field0, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(seg_src1_paddr, seg_field1, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(vec_src_paddr, vec_words, AI_IMC_VEC_VL4);

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("clean vsseg2e32 completes under a pending but disabled STIP and leaves an isolated segment-store image",
        segment_clean_complete && segment_image_isolated,
        "seg_vstart=%lx seg_vl=%lx pending=%lx trap_count=%lx dst=%x,%x,%x,%x guards=%lx,%lx",
        seg_vstart, seg_vl, pending_after_segment, ai_imc_trap_count,
        read32(seg_dst_paddr), read32(seg_dst_paddr + 4U),
        read32(seg_dst_paddr + 8U), read32(seg_dst_paddr + 12U),
        read64(seg_guard0_paddr), read64(seg_guard1_paddr));
    TEST_ASSERT("following ordinary vector store reports a precise store page fault without overwriting the completed segment feedback image",
        ordinary_vs_fault_precise && sources_still_split_after_fault,
        "loaded=%d cause=%lx tval=%lx expected=%lx vstart=%lx vl=%lx pending=%lx vec=%x,%x,%x,%x",
        vector_source_loaded, fault_cause, fault_tval, vec_fault_vaddr,
        fault_vstart, fault_vl, pending_after_fault,
        read32(vec_low_tail_paddr), read32(vec_low_tail_paddr + 4U),
        read32(vec_high_head_paddr), read32(vec_high_head_paddr + 4U));
    TEST_ASSERT("STIP handler observes and clears exactly one interrupt between the segment finish and ordinary vector-store retry without changing vstart or images",
        interrupt_logged_without_source_mix,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx vstart_after_int=%lx fault_vstart=%lx final_mip=%lx high=%x,%x",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], vstart_after_interrupt, fault_vstart,
        CSRR(CSR_MIP), read32(vec_high_head_paddr),
        read32(vec_high_head_paddr + 4U));
    TEST_ASSERT("after repair, retry from captured vstart writes only the ordinary vector-store suffix while the earlier segment source and destination stay intact",
        retry_no_fault && final_images_remain_source_split &&
            all_sources_intact,
        "retry_vstart=%lx retry_vl=%lx seg=%x,%x,%x,%x vec=%x,%x,%x,%x sources=%d guards=%lx,%lx,%lx,%lx",
        retry_vstart, retry_vl, read32(seg_dst_paddr),
        read32(seg_dst_paddr + 4U), read32(seg_dst_paddr + 8U),
        read32(seg_dst_paddr + 12U), read32(vec_low_tail_paddr),
        read32(vec_low_tail_paddr + 4U), read32(vec_high_head_paddr),
        read32(vec_high_head_paddr + 4U), all_sources_intact,
        read64(seg_guard0_paddr), read64(seg_guard1_paddr),
        read64(vec_low_guard_paddr), read64(vec_high_guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_vsegment_vsmerge_feedback_source_split_corner);
}

bool ai_interrupt_memory_hs_stip_vector_segment_load_fault_handler_spill_corner()
{
    TEST_START();

    const enum test_page low_page = VSRWX_GRWX;
    const enum test_page high_page = VSRWX_GRW;
    const enum test_page sentinel_page = SWITCH1;
    const enum test_page dump_page = SWITCH2;
    const uintptr_t src_vaddr = hs_page_base(low_page) + PAGE_SIZE - 8U;
    const uintptr_t fault_vaddr = hs_page_base(high_page);
    const uintptr_t low_tail_paddr = phys_page_base(low_page) +
        PAGE_SIZE - 8U;
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t sentinel0_paddr = phys_page_base(sentinel_page) + 0x9c0U;
    const uintptr_t sentinel1_paddr = phys_page_base(sentinel_page) + 0x9e0U;
    const uintptr_t sentinel0_vaddr = hs_page_base(sentinel_page) + 0x9c0U;
    const uintptr_t sentinel1_vaddr = hs_page_base(sentinel_page) + 0x9e0U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0x9c0U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0x9c0U;
    const uintptr_t handler_src = phys_page_base(SCRATCHPAD) + 0x9c0U;
    const uintptr_t handler_dst = phys_page_base(SCRATCHPAD) + 0x9e0U;
    const uintptr_t handler_save = phys_page_base(SCRATCHPAD) + 0xa00U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0xa20U;
    const uint32_t sentinel0[AI_IMC_VEC_VL4] = {
        0x51000000U, 0x51000001U, 0x51000002U, 0x51000003U
    };
    const uint32_t sentinel1[AI_IMC_VEC_VL4] = {
        0x52000000U, 0x52000001U, 0x52000002U, 0x52000003U
    };
    const uint32_t field0[AI_IMC_VEC_VL4] = {
        0x61000000U, 0x61000001U, 0x61000002U, 0x61000003U
    };
    const uint32_t field1[AI_IMC_VEC_VL4] = {
        0x62000000U, 0x62000001U, 0x62000002U, 0x62000003U
    };
    const uint16_t handler_words[AI_IMC_VEC_VL4] = {
        0x7101U, 0x7202U, 0x7303U, 0x7404U
    };
    const uint64_t guard_seed = 0x696d63766c736761ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t fault_vl = 0xffffffffffffffffULL;
    uint64_t dump_vstart0 = 0xffffffffffffffffULL;
    uint64_t dump_vl0 = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t retry_vl = 0xffffffffffffffffULL;
    uint64_t dump_vstart1 = 0xffffffffffffffffULL;
    uint64_t dump_vl1 = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(sentinel0_paddr + i * sizeof(uint32_t), sentinel0[i]);
        write32(sentinel1_paddr + i * sizeof(uint32_t), sentinel1[i]);
        write16(handler_src + i * sizeof(uint16_t), handler_words[i]);
        write16(handler_dst + i * sizeof(uint16_t), 0xddddU);
        write32(handler_save + i * sizeof(uint32_t), 0xeeee0000U + i);
        write32(dump_paddr + 2ULL * i * sizeof(uint32_t), 0xcccc0000U + i);
        write32(dump_paddr + (2ULL * i + 1ULL) * sizeof(uint32_t),
            0xdddd0000U + i);
    }
    write32(low_tail_paddr + 0U, field0[0]);
    write32(low_tail_paddr + 4U, field1[0]);
    for (uint64_t i = 0ULL; i < 6ULL; ++i) {
        write32(high_head_paddr + i * sizeof(uint32_t),
            (i & 1ULL) == 0ULL ? field0[(i / 2ULL) + 1ULL] :
                field1[(i / 2ULL) + 1ULL]);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_X);
    ai_imc_set_hs_leaf(sentinel_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(dump_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_load_seeded((const uint32_t *)sentinel0_vaddr,
        (const uint32_t *)sentinel1_vaddr, (const uint32_t *)src_vaddr,
        0ULL, &fault_vstart, &fault_vl);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool segment_load_fault_precise =
        excpt.triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == fault_vaddr &&
        fault_vstart == 1ULL &&
        fault_vl == AI_IMC_VEC_VL4 &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_dump_v8_v9((uint32_t *)dump_vaddr,
        &dump_vstart0, &dump_vl0);
    const bool fault_dump_no_exception = !excpt.triggered;

    goto_priv(PRIV_M);
    const bool fault_destination_group_preserved =
        fault_dump_no_exception &&
        dump_vstart0 == 0ULL &&
        dump_vl0 == AI_IMC_VEC_VL4 &&
        read32(dump_paddr + 0U) == field0[0] &&
        read32(dump_paddr + 4U) == field1[0] &&
        read32(dump_paddr + 8U) == sentinel0[1] &&
        read32(dump_paddr + 12U) == sentinel1[1] &&
        read32(dump_paddr + 16U) == sentinel0[2] &&
        read32(dump_paddr + 20U) == sentinel1[2] &&
        read32(dump_paddr + 24U) == sentinel0[3] &&
        read32(dump_paddr + 28U) == sentinel1[3];

    ai_imc_reset_trap_log();
    ai_imc_handler_vec_src_addr = handler_src;
    ai_imc_handler_vec_dst_addr = handler_dst;
    ai_imc_handler_vec_save_addr = handler_save;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_segment_spill_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool handler_vector_spill_isolated =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_vec_saved_vstart[0] == 0ULL &&
        ai_imc_handler_vec_handler_vl[0] >= AI_IMC_VEC_VL4 &&
        read32(handler_save + 0U) == field0[0] &&
        read32(handler_save + 4U) == field1[0] &&
        read32(handler_save + 8U) == sentinel0[1] &&
        read32(handler_save + 12U) == sentinel1[1] &&
        read16(handler_dst + 0U) == handler_words[0] &&
        read16(handler_dst + 2U) == handler_words[1] &&
        read16(handler_dst + 4U) == handler_words[2] &&
        read16(handler_dst + 6U) == handler_words[3] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(high_page, PTE_V | PTE_AD | PTE_RWX);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_load_resume((const uint32_t *)src_vaddr,
        fault_vstart, &retry_vstart, &retry_vl);
    const bool retry_no_exception = !excpt.triggered;
    TEST_SETUP_EXCEPT();
    ai_imc_vec_seg2e32_dump_v8_v9((uint32_t *)dump_vaddr,
        &dump_vstart1, &dump_vl1);
    const bool retry_dump_no_exception = !excpt.triggered;

    goto_priv(PRIV_M);
    const bool retry_destination_group_complete =
        retry_no_exception &&
        retry_dump_no_exception &&
        retry_vstart == 0ULL &&
        retry_vl == AI_IMC_VEC_VL4 &&
        dump_vstart1 == 0ULL &&
        dump_vl1 == AI_IMC_VEC_VL4 &&
        read32(dump_paddr + 0U) == field0[0] &&
        read32(dump_paddr + 4U) == field1[0] &&
        read32(dump_paddr + 8U) == field0[1] &&
        read32(dump_paddr + 12U) == field1[1] &&
        read32(dump_paddr + 16U) == field0[2] &&
        read32(dump_paddr + 20U) == field1[2] &&
        read32(dump_paddr + 24U) == field0[3] &&
        read32(dump_paddr + 28U) == field1[3] &&
        read16(handler_dst + 0U) == handler_words[0] &&
        read16(handler_dst + 6U) == handler_words[3] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    ai_imc_handler_vec_src_addr = 0ULL;
    ai_imc_handler_vec_dst_addr = 0ULL;
    ai_imc_handler_vec_save_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("vlseg2e32 load crossing a no-read page reports the first faulting segment element while STIP remains pending",
        segment_load_fault_precise,
        "cause=%lx tval=%lx expected=%lx vstart=%lx vl=%lx pending=%lx trap_count=%lx",
        fault_cause, fault_tval, fault_vaddr, fault_vstart,
        fault_vl, pending_after_fault, ai_imc_trap_count);
    TEST_ASSERT("after the segment load fault, only element0 field data is visible and later destination fields keep their sentinels",
        fault_destination_group_preserved,
        "dump=%x,%x,%x,%x,%x,%x,%x,%x vstart=%lx vl=%lx",
        read32(dump_paddr + 0U), read32(dump_paddr + 4U),
        read32(dump_paddr + 8U), read32(dump_paddr + 12U),
        read32(dump_paddr + 16U), read32(dump_paddr + 20U),
        read32(dump_paddr + 24U), read32(dump_paddr + 28U),
        dump_vstart0, dump_vl0);
    TEST_ASSERT("STIP handler vector spill uses an independent save/destination area and preserves the captured segment destination group",
        handler_vector_spill_isolated,
        "rounds=%lx trap_count=%lx cause=%lx save=%x,%x,%x,%x handler=%x,%x,%x,%x guard=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        read32(handler_save), read32(handler_save + 4U),
        read32(handler_save + 8U), read32(handler_save + 12U),
        read16(handler_dst), read16(handler_dst + 2U),
        read16(handler_dst + 4U), read16(handler_dst + 6U),
        read64(guard), CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the high page, retry from captured vstart completes every segment field without importing handler spill data",
        retry_destination_group_complete,
        "retry_vstart=%lx retry_vl=%lx dump=%x,%x,%x,%x,%x,%x,%x,%x handler0=%x handler3=%x guard=%lx",
        retry_vstart, retry_vl,
        read32(dump_paddr + 0U), read32(dump_paddr + 4U),
        read32(dump_paddr + 8U), read32(dump_paddr + 12U),
        read32(dump_paddr + 16U), read32(dump_paddr + 20U),
        read32(dump_paddr + 24U), read32(dump_paddr + 28U),
        read16(handler_dst), read16(handler_dst + 6U),
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_vector_segment_load_fault_handler_spill_corner);
}

bool ai_interrupt_memory_m_stip_ordered_indexed_duplicate_vstore_owner_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH1) + 0xf00U;
    const uintptr_t offsets = phys_page_base(SWITCH1) + 0xf40U;
    const uintptr_t src = phys_page_base(SWITCH1) + 0xf80U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xfc0U;
    const uint64_t offset_words[AI_IMC_VEC_VL4] = {
        0ULL, 4ULL, 0ULL, 0ULL
    };
    const uint32_t src_words[AI_IMC_VEC_VL4] = {
        0x11112222U, 0x33334444U, 0x55556666U, 0x77778888U
    };
    const uint64_t guard_seed = 0x696d636f6476aa01ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t final_vstart = 0xffffffffffffffffULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write64(offsets + i * sizeof(uint64_t), offset_words[i]);
        write32(src + i * sizeof(uint32_t), src_words[i]);
    }
    write32(base + 0U, 0xaaaa0000U);
    write32(base + 4U, 0xbbbb0001U);
    write32(base + 8U, 0xcccc0002U);
    write32(base + 12U, 0xdddd0003U);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_duplicate_store((const uint8_t *)base,
        (const uint64_t *)offsets, (const uint32_t *)src,
        &final_vstart);
    (void)ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    const bool interrupt_logged_once =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool duplicate_ordered_store_image =
        !excpt.triggered &&
        final_vstart == 0ULL &&
        read32(base + 0U) == src_words[3] &&
        read32(base + 4U) == 0xbbbb0001U &&
        read32(base + 8U) == 0xcccc0002U &&
        read32(base + 12U) == 0xdddd0003U &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP in the ordered indexed duplicate-store window is delivered once and does not reenter",
        interrupt_logged_once,
        "trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("ordered indexed duplicate-address vector store keeps the last active lane as the final owner and ignores the masked lane",
        duplicate_ordered_store_image,
        "vstart=%lx mem=%x,%x,%x,%x guard=%lx excpt=%d",
        final_vstart, read32(base), read32(base + 4U),
        read32(base + 8U), read32(base + 12U), read64(guard),
        excpt.triggered);

    TEST_END(ai_interrupt_memory_m_stip_ordered_indexed_duplicate_vstore_owner_corner);
}

bool ai_interrupt_memory_hs_stip_ordered_indexed_vload_fault_repair_vstart_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page aux_page = SWITCH2;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t elem0_offset = 0xa00U;
    const uintptr_t elem2_offset = 0xa10U;
    const uintptr_t elem3_offset = 0xa18U;
    const uintptr_t fault_offset = 0xa40U;
    const uintptr_t base_paddr = phys_page_base(base_page);
    const uintptr_t elem0_paddr = base_paddr + elem0_offset;
    const uintptr_t elem2_paddr = base_paddr + elem2_offset;
    const uintptr_t elem3_paddr = base_paddr + elem3_offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + fault_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + fault_offset;
    const uintptr_t offsets_vaddr = hs_page_base(aux_page) + 0xa80U;
    const uintptr_t offsets_paddr = phys_page_base(aux_page) + 0xa80U;
    const uintptr_t sentinel_vaddr = hs_page_base(aux_page) + 0xac0U;
    const uintptr_t sentinel_paddr = phys_page_base(aux_page) + 0xac0U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t guard_lo = elem0_paddr - sizeof(uint64_t);
    const uintptr_t guard_hi = elem3_paddr + sizeof(uint64_t);
    const uintptr_t fault_guard = fault_paddr + sizeof(uint64_t);
    const uint64_t elem0_value = 0x696d636f69786400ULL;
    const uint64_t fault_value = 0x696d636f69786411ULL;
    const uint64_t elem2_value = 0x696d636f69786422ULL;
    const uint64_t elem3_value = 0x696d636f69786433ULL;
    const uint64_t elem0_poison = 0xbad0000000000000ULL;
    const uint64_t guard_lo_seed = 0x696d636f69786d6cULL;
    const uint64_t guard_hi_seed = 0x696d636f69786d68ULL;
    const uint64_t fault_guard_seed = 0x696d636f69786667ULL;
    const uint64_t sentinel[AI_IMC_VEC_VL4] = {
        0x5151000000000000ULL,
        0x5151000000000001ULL,
        0x5151000000000002ULL,
        0x5151000000000003ULL
    };
    const uint64_t offsets[AI_IMC_VEC_VL4] = {
        (uint64_t)elem0_offset,
        (uint64_t)ai_imc_hs_page_offset(base_page, fault_page,
            fault_offset),
        (uint64_t)elem2_offset,
        (uint64_t)elem3_offset
    };
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t vstart_after_interrupt = 0xffffffffffffffffULL;
    uint64_t retry_vstart = 0xffffffffffffffffULL;
    uint64_t interrupt_rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(elem0_paddr, elem0_value);
    write64(fault_paddr, fault_value);
    write64(elem2_paddr, elem2_value);
    write64(elem3_paddr, elem3_value);
    write64(guard_lo, guard_lo_seed);
    write64(guard_hi, guard_hi_seed);
    write64(fault_guard, fault_guard_seed);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), offsets[i]);
        write64(sentinel_paddr + i * sizeof(uint64_t), sentinel[i]);
        write64(dump_paddr + i * sizeof(uint64_t),
            0xeeee000000000000ULL + i);
    }
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(fault_page, PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    const bool address_contract =
        base_vaddr + offsets[0] == hs_page_base(base_page) + elem0_offset &&
        base_vaddr + offsets[1] == fault_vaddr &&
        base_vaddr + offsets[2] == hs_page_base(base_page) + elem2_offset &&
        base_vaddr + offsets[3] == hs_page_base(base_page) + elem3_offset;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_load64_seeded(
        (const uint64_t *)sentinel_vaddr,
        (const uint64_t *)offsets_vaddr,
        (const uint8_t *)base_vaddr, 0ULL);
    fault_vstart = CSRR(CSR_VSTART);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool first_fault_is_oldest =
        address_contract &&
        excpt.triggered &&
        fault_cause == CAUSE_LPF &&
        fault_tval == fault_vaddr &&
        fault_vstart == 1ULL &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        read64(guard_lo) == guard_lo_seed &&
        read64(guard_hi) == guard_hi_seed &&
        read64(fault_guard) == fault_guard_seed;

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_dump_v6((uint64_t *)dump_vaddr);
    const bool dump_after_fault_ok = !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL;

    goto_priv(PRIV_M);
    const bool prefix_only_after_fault =
        dump_after_fault_ok &&
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == sentinel[1] &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == sentinel[2] &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == sentinel[3] &&
        read64(guard_lo) == guard_lo_seed &&
        read64(guard_hi) == guard_hi_seed &&
        read64(fault_guard) == fault_guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    vstart_after_interrupt = CSRR(CSR_VSTART);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_preserves_fault_contract =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        vstart_after_interrupt == 0ULL &&
        read64(guard_lo) == guard_lo_seed &&
        read64(guard_hi) == guard_hi_seed &&
        read64(fault_guard) == fault_guard_seed;

    write64(elem0_paddr, elem0_poison);

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_load64_resume(
        (const uint64_t *)offsets_vaddr,
        (const uint8_t *)base_vaddr, fault_vstart);
    retry_vstart = CSRR(CSR_VSTART);
    const bool retry_no_fault =
        !excpt.triggered &&
        retry_vstart == 0ULL;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_dump_v6((uint64_t *)dump_vaddr);
    const bool final_dump_ok =
        !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL;

    goto_priv(PRIV_M);
    const bool retry_uses_captured_vstart =
        retry_no_fault &&
        final_dump_ok &&
        read64(dump_paddr + 0U * sizeof(uint64_t)) == elem0_value &&
        read64(dump_paddr + 0U * sizeof(uint64_t)) != elem0_poison &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == fault_value &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == elem2_value &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == elem3_value &&
        read64(elem0_paddr) == elem0_poison &&
        read64(guard_lo) == guard_lo_seed &&
        read64(guard_hi) == guard_hi_seed &&
        read64(fault_guard) == fault_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("ordered indexed vector load reports the oldest active fault lane while a disabled STIP remains pending",
        first_fault_is_oldest,
        "addr_contract=%d cause=%lx tval=%lx expected=%lx vstart=%lx pending=%lx traps=%lx",
        address_contract, fault_cause, fault_tval, fault_vaddr,
        fault_vstart, pending_after_fault, ai_imc_trap_count);
    TEST_ASSERT("before repair, only the prefix lane is committed and suffix lanes keep their seeded image",
        prefix_only_after_fault,
        "dump=%lx,%lx,%lx,%lx guards=%lx,%lx,%lx",
        read64(dump_paddr), read64(dump_paddr + 8U),
        read64(dump_paddr + 16U), read64(dump_paddr + 24U),
        read64(guard_lo), read64(guard_hi), read64(fault_guard));
    TEST_ASSERT("the preserved STIP is delivered exactly once and does not overwrite the captured vector fault state",
        interrupt_preserves_fault_contract,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx vstart_after_int=%lx final_mip=%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], vstart_after_interrupt, CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the fault leaf, retry from captured vstart keeps prefix data and loads the repaired lane plus suffix lanes",
        retry_uses_captured_vstart,
        "retry_vstart=%lx dump=%lx,%lx,%lx,%lx elem0_mem=%lx guards=%lx,%lx,%lx",
        retry_vstart, read64(dump_paddr), read64(dump_paddr + 8U),
        read64(dump_paddr + 16U), read64(dump_paddr + 24U),
        read64(elem0_paddr), read64(guard_lo), read64(guard_hi),
        read64(fault_guard));

    TEST_END(ai_interrupt_memory_hs_stip_ordered_indexed_vload_fault_repair_vstart_corner);
}

bool ai_interrupt_memory_hs_stip_vlq_fault_recovery_new_load_generation_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page fault_page = VSRWX_GRW;
    const enum test_page aux_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page);
    const uintptr_t elem0_offset = 0xb00U;
    const uintptr_t fault_offset = 0xb40U;
    const uintptr_t elem2_offset = 0xb80U;
    const uintptr_t elem3_offset = 0xbc0U;
    const uintptr_t elem0_paddr = phys_page_base(base_page) + elem0_offset;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + fault_offset;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + fault_offset;
    const uintptr_t elem2_paddr = phys_page_base(base_page) + elem2_offset;
    const uintptr_t elem3_paddr = phys_page_base(base_page) + elem3_offset;
    const uintptr_t offsets_vaddr = hs_page_base(aux_page) + 0xb00U;
    const uintptr_t offsets_paddr = phys_page_base(aux_page) + 0xb00U;
    const uintptr_t sentinel_vaddr = hs_page_base(aux_page) + 0xb40U;
    const uintptr_t sentinel_paddr = phys_page_base(aux_page) + 0xb40U;
    const uintptr_t dump_vaddr = hs_page_base(aux_page) + 0xb80U;
    const uintptr_t dump_paddr = phys_page_base(aux_page) + 0xb80U;
    const uintptr_t guard = phys_page_base(aux_page) + 0xbc0U;
    const uint64_t old_values[AI_IMC_VEC_VL4] = {
        0x696d63766c716f30ULL,
        0x696d63766c716f31ULL,
        0x696d63766c716f32ULL,
        0x696d63766c716f33ULL
    };
    const uint64_t new_values[AI_IMC_VEC_VL4] = {
        0x696d63766c716e40ULL,
        0x696d63766c716e41ULL,
        0x696d63766c716e42ULL,
        0x696d63766c716e43ULL
    };
    const uint64_t sentinel[AI_IMC_VEC_VL4] = {
        0x5152515251525100ULL,
        0x5152515251525101ULL,
        0x5152515251525102ULL,
        0x5152515251525103ULL
    };
    const uint64_t offsets[AI_IMC_VEC_VL4] = {
        (uint64_t)elem0_offset,
        (uint64_t)ai_imc_hs_page_offset(base_page, fault_page,
            fault_offset),
        (uint64_t)elem2_offset,
        (uint64_t)elem3_offset
    };
    const uint64_t guard_seed = 0x696d63766c716775ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_vstart = 0xffffffffffffffffULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(elem0_paddr, old_values[0]);
    write64(fault_paddr, old_values[1]);
    write64(elem2_paddr, old_values[2]);
    write64(elem3_paddr, old_values[3]);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write64(offsets_paddr + i * sizeof(uint64_t), offsets[i]);
        write64(sentinel_paddr + i * sizeof(uint64_t), sentinel[i]);
        write64(dump_paddr + i * sizeof(uint64_t),
            0xeeeeeeeeeeee0000ULL + i);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(base_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(fault_page, PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(aux_page, PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_load64_seeded(
        (const uint64_t *)sentinel_vaddr,
        (const uint64_t *)offsets_vaddr,
        (const uint8_t *)base_vaddr, 0ULL);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_vstart = CSRR(CSR_VSTART);

    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_dump_v6((uint64_t *)dump_vaddr);
    const bool old_redirect_keeps_prefix_only =
        fault_cause == CAUSE_LPF &&
        fault_tval == fault_vaddr &&
        fault_vstart == 1ULL &&
        !excpt.triggered &&
        read64(dump_paddr + 0U * sizeof(uint64_t)) == old_values[0] &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == sentinel[1] &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == sentinel[2] &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == sentinel[3] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    write64(elem0_paddr, new_values[0]);
    write64(fault_paddr, new_values[1]);
    write64(elem2_paddr, new_values[2]);
    write64(elem3_paddr, new_values[3]);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    ai_imc_set_hs_leaf(fault_page, PTE_V | PTE_AD | PTE_RWX);
    sfence_vma();
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_load64_seeded(
        (const uint64_t *)sentinel_vaddr,
        (const uint64_t *)offsets_vaddr,
        (const uint8_t *)base_vaddr, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_ordered_indexed_dump_v6((uint64_t *)dump_vaddr);
    const bool new_generation_load_not_canceled_by_old_redirect =
        !excpt.triggered &&
        CSRR(CSR_VSTART) == 0ULL &&
        read64(dump_paddr + 0U * sizeof(uint64_t)) == new_values[0] &&
        read64(dump_paddr + 1U * sizeof(uint64_t)) == new_values[1] &&
        read64(dump_paddr + 2U * sizeof(uint64_t)) == new_values[2] &&
        read64(dump_paddr + 3U * sizeof(uint64_t)) == new_values[3] &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    const bool interrupt_between_generations_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("old VLQ indexed-load generation redirects on lane1 fault and leaves only the prefix lane visible",
        old_redirect_keeps_prefix_only,
        "cause=%lx tval=%lx expected=%lx vstart=%lx dump=%lx,%lx,%lx,%lx guard=%lx",
        fault_cause, fault_tval, fault_vaddr, fault_vstart,
        read64(dump_paddr), read64(dump_paddr + 8U),
        read64(dump_paddr + 16U), read64(dump_paddr + 24U),
        read64(guard));
    TEST_ASSERT("STIP between VLQ redirect recovery and the new load generation is delivered once and clears pending state",
        interrupt_between_generations_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the fault leaf, the new vector-load generation fills every lane with new payloads and no stale prefix/sentinel survives",
        new_generation_load_not_canceled_by_old_redirect,
        "dump=%lx,%lx,%lx,%lx expected=%lx,%lx,%lx,%lx guard=%lx",
        read64(dump_paddr), read64(dump_paddr + 8U),
        read64(dump_paddr + 16U), read64(dump_paddr + 24U),
        new_values[0], new_values[1], new_values[2], new_values[3],
        read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_vlq_fault_recovery_new_load_generation_corner);
}

bool ai_interrupt_memory_hs_stip_sfence_remap_new_pa_visible_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page new_page = SWITCH2;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t vaddr = hs_page_base(va_page) + 0xc00U;
    const uintptr_t old_paddr = phys_page_base(va_page) + 0xc00U;
    const uintptr_t new_paddr = phys_page_base(new_page) + 0xc00U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard = new_paddr + sizeof(uint64_t);
    const uintptr_t handler_guard = phys_page_base(guard_page) + 0xc20U;
    const uint64_t old_seed = 0x696d6372666f6c64ULL;
    const uint64_t new_seed = 0x696d6372666e6577ULL;
    const uint64_t post_payload = 0x696d63726673746fULL;
    const uint64_t old_guard_seed = 0x696d637266676f6cULL;
    const uint64_t new_guard_seed = 0x696d637266676e65ULL;
    const uint64_t handler_guard_seed = 0x696d637266686c67ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t first_load = 0ULL;
    uint64_t remap_load = 0ULL;
    uint64_t post_load = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(old_guard, old_guard_seed);
    write64(new_guard, new_guard_seed);
    write64(handler_guard, handler_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(va_page, va_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(new_page, new_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(guard_page, guard_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    first_load = ai_imc_ld_probe(vaddr, 0ULL);
    const bool old_mapping_visible =
        !excpt.triggered &&
        first_load == old_seed;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_write_hs_leaf_to_page(va_page, new_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    remap_load = ai_imc_ld_probe(vaddr, 0ULL);
    sd(vaddr, post_payload);
    post_load = ai_imc_ld_probe(vaddr, 0ULL);
    const bool new_mapping_visible =
        !excpt.triggered &&
        remap_load == new_seed &&
        post_load == post_payload;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool physical_images_isolated =
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == post_payload &&
        read64(old_guard) == old_guard_seed &&
        read64(new_guard) == new_guard_seed &&
        read64(handler_guard) == handler_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the remap probe first reads the old physical payload through the target HS VA",
        old_mapping_visible,
        "first=%lx old_seed=%lx", first_load, old_seed);
    TEST_ASSERT("STIP between PTE update and SFENCE.VMA is logged exactly once and does not change the target memory",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after SFENCE.VMA, the same VA uses the new physical page and stores only to the new backing image",
        new_mapping_visible && physical_images_isolated,
        "remap=%lx post=%lx old_pa=%lx new_pa=%lx guards=%lx,%lx handler_guard=%lx",
        remap_load, post_load, read64(old_paddr), read64(new_paddr),
        read64(old_guard), read64(new_guard), read64(handler_guard));

    TEST_END(ai_interrupt_memory_hs_stip_sfence_remap_new_pa_visible_corner);
}

bool ai_interrupt_memory_hs_stip_same_pa_alias_store_load_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = VSRWX_GX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xc80U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0xc80U;
    const uintptr_t backing_paddr = phys_page_base(base_page) + 0xc80U;
    const uintptr_t adjacent_paddr = backing_paddr + sizeof(uint64_t);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0xc80U;
    const uint64_t seed = 0x696d63616c736565ULL;
    const uint64_t alias_payload = 0x696d63616c73746fULL;
    const uint64_t base_payload = 0x696d63616c626173ULL;
    const uint64_t adjacent_seed = 0x696d63616c61646aULL;
    const uint64_t guard_seed = 0x696d63616c677561ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t base_before = 0ULL;
    uint64_t alias_after_store = 0ULL;
    uint64_t base_after_alias_store = 0ULL;
    uint64_t alias_after_base_store = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(backing_paddr, seed);
    write64(adjacent_paddr, adjacent_seed);
    write64(guard_paddr, guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(base_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(alias_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(guard_page, guard_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    base_before = ai_imc_ld_probe(base_vaddr, 0ULL);
    sd(alias_vaddr, alias_payload);
    alias_after_store = ai_imc_ld_probe(alias_vaddr, 0ULL);
    const bool alias_store_seen_by_alias =
        !excpt.triggered &&
        base_before == seed &&
        alias_after_store == alias_payload;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    base_after_alias_store = ai_imc_ld_probe(base_vaddr, 0ULL);
    sd(base_vaddr, base_payload);
    alias_after_base_store = ai_imc_ld_probe(alias_vaddr, 0ULL);
    const bool same_pa_bidirectional_visible =
        !excpt.triggered &&
        base_after_alias_store == alias_payload &&
        alias_after_base_store == base_payload;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool backing_image_ok =
        read64(backing_paddr) == base_payload &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(guard_paddr) == guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the alias VA store updates the shared physical backing before interrupt delivery",
        alias_store_seen_by_alias,
        "base_before=%lx alias_after_store=%lx seed=%lx payload=%lx",
        base_before, alias_after_store, seed, alias_payload);
    TEST_ASSERT("STIP between alias store and base load is logged exactly once and clears pending state",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after interrupt return, base and alias VAs observe the same PA image without corrupting adjacent guards",
        same_pa_bidirectional_visible && backing_image_ok,
        "base_after_alias=%lx alias_after_base=%lx backing=%lx adjacent=%lx guard=%lx",
        base_after_alias_store, alias_after_base_store,
        read64(backing_paddr), read64(adjacent_paddr),
        read64(guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_same_pa_alias_store_load_corner);
}

bool ai_interrupt_memory_hs_stip_same_pa_alias_lrsc_handler_load_corner()
{
    TEST_START();

    const enum test_page base_page = SWITCH1;
    const enum test_page alias_page = VSRWX_GX;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t base_vaddr = hs_page_base(base_page) + 0xcc0U;
    const uintptr_t alias_vaddr = hs_page_base(alias_page) + 0xcc0U;
    const uintptr_t backing_paddr = phys_page_base(base_page) + 0xcc0U;
    const uintptr_t adjacent_paddr = backing_paddr + sizeof(uint64_t);
    const uintptr_t guard_paddr = phys_page_base(guard_page) + 0xcc0U;
    const uint64_t seed = 0x696d636c73616c72ULL;
    const uint64_t sc_payload = 0x696d636c73616c53ULL;
    const uint64_t fresh_seed = 0x696d636c73616672ULL;
    const uint64_t fresh_payload = 0x696d636c73616653ULL;
    const uint64_t adjacent_seed = 0x696d636c73616164ULL;
    const uint64_t guard_seed = 0x696d636c73616764ULL;
    const uint64_t marker_value = 0x696d636c73616d6bULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t lr_value = 0ULL;
    uint64_t alias_probe_before = 0ULL;
    uint64_t sc_status = 1ULL;
    uint64_t base_after_sc = 0ULL;
    uint64_t alias_after_sc = 0ULL;
    uint64_t fresh_lr = 0ULL;
    uint64_t fresh_sc_status = 1ULL;
    uint64_t fresh_alias_after_sc = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(backing_paddr, seed);
    write64(adjacent_paddr, adjacent_seed);
    write64(guard_paddr, guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(base_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(alias_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(guard_page, guard_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    lr_value = ai_imc_lr_d_addr(base_vaddr, 0ULL);
    alias_probe_before = ai_imc_ld_probe(alias_vaddr, 0ULL);
    const bool alias_setup_visible =
        !excpt.triggered &&
        lr_value == seed &&
        alias_probe_before == seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = backing_paddr;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    sc_status = ai_imc_sc_d_addr(base_vaddr, sc_payload);
    base_after_sc = ai_imc_ld_probe(base_vaddr, 0ULL);
    alias_after_sc = ai_imc_ld_probe(alias_vaddr, 0ULL);
    const bool lrsc_event_set_consistent =
        !excpt.triggered &&
        ((sc_status == 0ULL &&
          base_after_sc == sc_payload &&
          alias_after_sc == sc_payload) ||
         (sc_status != 0ULL &&
          base_after_sc == seed &&
          alias_after_sc == seed));

    goto_priv(PRIV_M);
    const uint64_t expected_after_sc =
        sc_status == 0ULL ? sc_payload : seed;
    const bool interrupt_handler_alias_probe =
        alias_setup_visible &&
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_probe_value[0] == seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(backing_paddr) == expected_after_sc &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(guard_paddr) == guard_seed;

    write64(backing_paddr, fresh_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    fresh_lr = ai_imc_lr_d_addr(alias_vaddr, 0ULL);
    fresh_sc_status = ai_imc_sc_d_addr(alias_vaddr, fresh_payload);
    fresh_alias_after_sc = ai_imc_ld_probe(base_vaddr, 0ULL);
    const bool fresh_alias_lrsc_recovers =
        !excpt.triggered &&
        fresh_lr == fresh_seed &&
        fresh_sc_status == 0ULL &&
        fresh_alias_after_sc == fresh_payload;

    goto_priv(PRIV_M);
    write64(guard_paddr, marker_value);
    const bool final_backing_and_guards_ok =
        read64(backing_paddr) == fresh_payload &&
        read64(adjacent_paddr) == adjacent_seed &&
        read64(guard_paddr) == marker_value;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("LR through the base VA and load through the alias VA observe the same PA seed before interrupt",
        alias_setup_visible,
        "lr=%lx alias_probe=%lx seed=%lx",
        lr_value, alias_probe_before, seed);
    TEST_ASSERT("STIP handler probes the shared backing exactly once and leaves alias LR/SC source state coherent",
        interrupt_handler_alias_probe,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx expected_probe=%lx backing=%lx expected=%lx mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], seed, read64(backing_paddr),
        expected_after_sc, CSRR(CSR_MIP));
    TEST_ASSERT("SC after same-PA alias handler traffic keeps status and final PA image in the same legal event set",
        lrsc_event_set_consistent,
        "sc=%lx base_after=%lx alias_after=%lx seed=%lx payload=%lx",
        sc_status, base_after_sc, alias_after_sc, seed, sc_payload);
    TEST_ASSERT("a fresh LR/SC through the alias VA succeeds after the interrupted event and proves the shared PA can still commit once",
        fresh_alias_lrsc_recovers && final_backing_and_guards_ok,
        "fresh_lr=%lx fresh_sc=%lx base_after=%lx backing=%lx adjacent=%lx guard=%lx",
        fresh_lr, fresh_sc_status, fresh_alias_after_sc,
        read64(backing_paddr), read64(adjacent_paddr),
        read64(guard_paddr));

    TEST_END(ai_interrupt_memory_hs_stip_same_pa_alias_lrsc_handler_load_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_same_alias_side_effect_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t target_vaddr = hs_page_base(nc_page) + 0xd00U;
    const uintptr_t guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = target_vaddr + 2U * sizeof(uint64_t);
    const uint64_t seed = 0x696d63706e637301ULL;
    const uint64_t update = 0x696d63706e637355ULL;
    const uint64_t guard_seed = 0x696d63706e6367aaULL;
    const uint64_t marker_value = 0x696d63706e636d6bULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t seed_readback = 0ULL;
    uint64_t guard_readback = 0ULL;
    uint64_t post_interrupt_read = 0ULL;
    uint64_t final_readback = 0ULL;
    uint64_t final_guard = 0ULL;
    uint64_t final_marker = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1, 1ULL);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    sd(target_vaddr, seed);
    sd(guard_vaddr, guard_seed);
    sd(marker_vaddr, 0ULL);
    fence_iorw_iorw();
    seed_readback = ai_imc_ld_probe(target_vaddr, 0ULL);
    guard_readback = ai_imc_ld_probe(guard_vaddr, 0ULL);
    const bool seeded_and_checked_by_same_nc_alias =
        !excpt.triggered &&
        seed_readback == seed &&
        guard_readback == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    post_interrupt_read = ai_imc_ld_probe(target_vaddr, 0ULL);
    sd(target_vaddr, update);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    final_readback = ai_imc_ld_probe(target_vaddr, 0ULL);
    final_guard = ai_imc_ld_probe(guard_vaddr, 0ULL);
    final_marker = ai_imc_ld_probe(marker_vaddr, 0ULL);
    const bool updated_and_checked_by_same_nc_alias =
        !excpt.triggered &&
        post_interrupt_read == seed &&
        final_readback == update &&
        final_guard == guard_seed &&
        final_marker == marker_value;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC target and guard are seeded and observed through the same NC alias before interrupt delivery",
        seeded_and_checked_by_same_nc_alias,
        "seed_readback=%lx guard_readback=%lx seed=%lx guard=%lx",
        seed_readback, guard_readback, seed, guard_seed);
    TEST_ASSERT("STIP between PBMT=NC alias episodes is logged exactly once and source pending is cleared",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("after interrupt return, only the same PBMT=NC alias is used as the side-effect oracle for target, guard, and marker",
        updated_and_checked_by_same_nc_alias,
        "post=%lx final=%lx guard=%lx marker=%lx update=%lx",
        post_interrupt_read, final_readback, final_guard, final_marker,
        update);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_same_alias_side_effect_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_old_new_request_owner_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const uintptr_t old_vaddr = hs_page_base(nc_page) + 0xd40U;
    const uintptr_t new_vaddr = old_vaddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = old_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = old_vaddr + 3U * sizeof(uint64_t);
    const uint64_t old_seed = 0x696d63706e6f6c64ULL;
    const uint64_t new_seed = 0x696d63706e6e6577ULL;
    const uint64_t old_update = 0x696d63706e6f6c55ULL;
    const uint64_t new_update = 0x696d63706e6e6555ULL;
    const uint64_t guard_seed = 0x696d63706e7267aaULL;
    const uint64_t marker_value = 0x696d63706e726d6bULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t old_before = 0ULL;
    uint64_t new_before = 0ULL;
    uint64_t old_after_update = 0ULL;
    uint64_t new_after_interrupt = 0ULL;
    uint64_t old_after_new = 0ULL;
    uint64_t new_after_update = 0ULL;
    uint64_t guard_after = 0ULL;
    uint64_t marker_after = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1, 1ULL);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    sd(old_vaddr, old_seed);
    sd(new_vaddr, new_seed);
    sd(guard_vaddr, guard_seed);
    sd(marker_vaddr, 0ULL);
    fence_iorw_iorw();
    old_before = ai_imc_ld_probe(old_vaddr, 0ULL);
    new_before = ai_imc_ld_probe(new_vaddr, 0ULL);
    sd(old_vaddr, old_update);
    fence_iorw_iorw();
    old_after_update = ai_imc_ld_probe(old_vaddr, 0ULL);
    const bool old_episode_self_contained =
        !excpt.triggered &&
        old_before == old_seed &&
        new_before == new_seed &&
        old_after_update == old_update;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    new_after_interrupt = ai_imc_ld_probe(new_vaddr, 0ULL);
    sd(new_vaddr, new_update);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    old_after_new = ai_imc_ld_probe(old_vaddr, 0ULL);
    new_after_update = ai_imc_ld_probe(new_vaddr, 0ULL);
    guard_after = ai_imc_ld_probe(guard_vaddr, 0ULL);
    marker_after = ai_imc_ld_probe(marker_vaddr, 0ULL);
    const bool new_episode_not_rebound_to_old_owner =
        !excpt.triggered &&
        new_after_interrupt == new_seed &&
        old_after_new == old_update &&
        new_after_update == new_update &&
        guard_after == guard_seed &&
        marker_after == marker_value;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("older PBMT=NC request episode updates only the old slot before interrupt delivery",
        old_episode_self_contained,
        "old_before=%lx new_before=%lx old_after=%lx",
        old_before, new_before, old_after_update);
    TEST_ASSERT("STIP between old and new PBMT=NC request episodes is logged exactly once and clears pending state",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("post-interrupt PBMT=NC request binds to the new slot and does not inherit old owner data or handler log data",
        new_episode_not_rebound_to_old_owner &&
        new_update != old_update &&
        new_update != ai_imc_handler_log_page[0],
        "new_after_irq=%lx old_after_new=%lx new_after=%lx guard=%lx marker=%lx log=%lx",
        new_after_interrupt, old_after_new, new_after_update,
        guard_after, marker_after, ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_old_new_request_owner_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_replay_direct_progress_smoke_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t offset = 0xf40U;
    const uintptr_t replay_vaddr = hs_page_base(nc_page) + offset;
    const uintptr_t replay_paddr = phys_page_base(nc_page) + offset;
    const uintptr_t direct_vaddr = replay_vaddr + sizeof(uint64_t);
    const uintptr_t direct_paddr = replay_paddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = replay_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t guard_paddr = replay_paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + offset;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t replay_seed = 0x696d63706e727079ULL;
    const uint64_t direct_seed = 0x696d63706e646972ULL;
    const uint64_t direct_update = 0x696d63706e646955ULL;
    const uint64_t guard_seed = 0x696d63706e7267bbULL;
    const uint64_t marker_value = 0x696d63706e726d64ULL;
    const uint64_t marker_guard_seed = 0x696d63706e726d67ULL;
    const uint64_t replay_sentinel = 0x696d63706e73656eULL;
    const uint64_t direct_sentinel = 0x696d63706e736564ULL;
    const pte_t no_read_nc_leaf =
        PTE_V | PTE_AD | PTE_X | PTE_Pbmt1;
    const pte_t legal_nc_leaf =
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    const pte_t legal_cacheable_leaf =
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t replay_value = replay_sentinel;
    uint64_t direct_before = direct_sentinel;
    uint64_t direct_after = direct_sentinel;
    uint64_t replay_after_direct = replay_sentinel;
    uint64_t guard_after = 0ULL;
    uint64_t marker_after = 0ULL;
    uint64_t marker_guard_after = 0ULL;
    uint64_t fault_trap_count = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_mip = 0ULL;
    uint64_t pte_after_repair = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    write64(replay_paddr, replay_seed);
    write64(direct_paddr, direct_seed);
    write64(guard_paddr, guard_seed);
    write64(marker_paddr, 0ULL);
    write64(marker_guard_paddr, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        no_read_nc_leaf, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        legal_cacheable_leaf, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_LPF);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(nc_page);
    ai_imc_block_copy_pte_value =
        (phys_page_base(nc_page) >> 2U) | legal_nc_leaf;
    ai_imc_block_copy_restore_mtvec = saved_mtvec;
    CSRW(CSR_MTVEC,
        (uintptr_t)ai_imc_mtvec_block_copy_repair_handler);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    replay_value = ai_imc_ld_probe(replay_vaddr, replay_sentinel);

    goto_priv(PRIV_M);
    fault_trap_count = ai_imc_trap_count;
    fault_cause = ai_imc_trap_cause[0];
    fault_tval = ai_imc_trap_tval[0];
    fault_mip = ai_imc_trap_mip[0];
    pte_after_repair = *ai_imc_hs_leaf_entry(nc_page);
    const bool replay_like_nc_load_repaired_once =
        !excpt.triggered &&
        fault_trap_count == 1ULL &&
        fault_cause == CAUSE_LPF &&
        fault_tval == replay_vaddr &&
        (fault_mip & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_block_copy_repairs == 1ULL &&
        pte_after_repair ==
            ((phys_page_base(nc_page) >> 2U) | legal_nc_leaf) &&
        replay_value == replay_seed &&
        read64(replay_paddr) == replay_seed &&
        read64(direct_paddr) == direct_seed &&
        read64(guard_paddr) == guard_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    const bool pending_stip_after_replay_delivered_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    direct_before = ai_imc_ld_probe(direct_vaddr, direct_sentinel);
    sd(direct_vaddr, direct_update);
    fence_iorw_iorw();
    direct_after = ai_imc_ld_probe(direct_vaddr, direct_sentinel);
    replay_after_direct = ai_imc_ld_probe(replay_vaddr, replay_sentinel);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    guard_after = ai_imc_ld_probe(guard_vaddr, 0ULL);
    marker_after = ai_imc_ld_probe(marker_vaddr, 0ULL);
    marker_guard_after = ai_imc_ld_probe(marker_guard_vaddr, 0ULL);
    const bool post_interrupt_direct_nc_episode_progresses =
        !excpt.triggered &&
        direct_before == direct_seed &&
        direct_after == direct_update &&
        replay_after_direct == replay_seed &&
        guard_after == guard_seed &&
        marker_after == marker_value &&
        marker_guard_after == marker_guard_seed &&
        direct_update != replay_seed &&
        direct_update != ai_imc_handler_log_page[1];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr = 0ULL;
    ai_imc_block_copy_pte_value = 0ULL;
    ai_imc_block_copy_restore_mtvec = 0ULL;
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC replay-like load page fault is repaired once and retries to its original slot while STIP remains pending",
        replay_like_nc_load_repaired_once,
        "trap_count=%lx cause=%lx tval=%lx expected=%lx mip=%lx repairs=%lx pte=%lx replay=%lx direct=%lx guard=%lx",
        fault_trap_count, fault_cause, fault_tval, replay_vaddr,
        fault_mip, ai_imc_block_copy_repairs, pte_after_repair,
        replay_value, read64(direct_paddr), read64(guard_paddr));
    TEST_ASSERT("the pending STIP after the replay-like PBMT=NC repair is logged once and clears source state before the direct request episode",
        pending_stip_after_replay_delivered_once,
        "rounds=%lx trap_count=%lx causes=%lx,%lx mip1=%lx log1=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_mip[1],
        ai_imc_handler_log_page[1], CSRR(CSR_MIP));
    TEST_ASSERT("post-interrupt direct PBMT=NC load/store episode completes on the direct slot without consuming the repaired replay slot or handler log payload",
        post_interrupt_direct_nc_episode_progresses,
        "direct_before=%lx direct_after=%lx replay_after=%lx guard=%lx marker=%lx marker_guard=%lx log1=%lx",
        direct_before, direct_after, replay_after_direct, guard_after,
        marker_after, marker_guard_after, ai_imc_handler_log_page[1]);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_replay_direct_progress_smoke_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_replay_needfree_progress_smoke_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t offset = 0xf80U;
    const uintptr_t replay_vaddr = hs_page_base(nc_page) + offset;
    const uintptr_t replay_paddr = phys_page_base(nc_page) + offset;
    const uintptr_t new0_vaddr = replay_vaddr + sizeof(uint64_t);
    const uintptr_t new0_paddr = replay_paddr + sizeof(uint64_t);
    const uintptr_t new1_vaddr = replay_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t new1_paddr = replay_paddr + 2U * sizeof(uint64_t);
    const uintptr_t guard_vaddr = replay_vaddr + 3U * sizeof(uint64_t);
    const uintptr_t guard_paddr = replay_paddr + 3U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + offset;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uintptr_t marker_guard_paddr = marker_paddr + sizeof(uint64_t);
    const uint64_t replay_seed = 0x696d63706e6e6630ULL;
    const uint64_t new0_seed = 0x696d63706e6e3030ULL;
    const uint64_t new1_seed = 0x696d63706e6e3131ULL;
    const uint64_t new0_payload = 0x696d63706e6e30a0ULL;
    const uint64_t new1_payload = 0x696d63706e6e31b1ULL;
    const uint64_t guard_seed = 0x696d63706e6e67ccULL;
    const uint64_t marker_value = 0x696d63706e6e6d6bULL;
    const uint64_t marker_guard_seed = 0x696d63706e6e6d67ULL;
    const uint64_t replay_sentinel = 0x696d63706e6e7365ULL;
    const uint64_t new_sentinel = 0x696d63706e6e7377ULL;
    const pte_t no_read_nc_leaf =
        PTE_V | PTE_AD | PTE_X | PTE_Pbmt1;
    const pte_t legal_nc_leaf =
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    const pte_t legal_cacheable_leaf =
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t replay_value = replay_sentinel;
    uint64_t new0_before = new_sentinel;
    uint64_t new0_after = new_sentinel;
    uint64_t new1_before = new_sentinel;
    uint64_t new1_after = new_sentinel;
    uint64_t replay_after_new = replay_sentinel;
    uint64_t guard_after = 0ULL;
    uint64_t marker_after = 0ULL;
    uint64_t marker_guard_after = 0ULL;
    uint64_t fault_trap_count = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_mip = 0ULL;
    uint64_t pte_after_repair = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    write64(replay_paddr, replay_seed);
    write64(new0_paddr, new0_seed);
    write64(new1_paddr, new1_seed);
    write64(guard_paddr, guard_seed);
    write64(marker_paddr, 0ULL);
    write64(marker_guard_paddr, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        no_read_nc_leaf, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        legal_cacheable_leaf, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_LPF);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(nc_page);
    ai_imc_block_copy_pte_value =
        (phys_page_base(nc_page) >> 2U) | legal_nc_leaf;
    ai_imc_block_copy_restore_mtvec = saved_mtvec;
    CSRW(CSR_MTVEC,
        (uintptr_t)ai_imc_mtvec_block_copy_repair_handler);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    replay_value = ai_imc_ld_probe(replay_vaddr, replay_sentinel);

    goto_priv(PRIV_M);
    fault_trap_count = ai_imc_trap_count;
    fault_cause = ai_imc_trap_cause[0];
    fault_tval = ai_imc_trap_tval[0];
    fault_mip = ai_imc_trap_mip[0];
    pte_after_repair = *ai_imc_hs_leaf_entry(nc_page);
    const bool replay_entry_repaired_with_pending_interrupt =
        !excpt.triggered &&
        fault_trap_count == 1ULL &&
        fault_cause == CAUSE_LPF &&
        fault_tval == replay_vaddr &&
        (fault_mip & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_block_copy_repairs == 1ULL &&
        pte_after_repair ==
            ((phys_page_base(nc_page) >> 2U) | legal_nc_leaf) &&
        replay_value == replay_seed;

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    const bool interrupt_after_replay_repair_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    new0_before = ai_imc_ld_probe(new0_vaddr, new_sentinel);
    sd(new0_vaddr, new0_payload);
    new0_after = ai_imc_ld_probe(new0_vaddr, new_sentinel);
    new1_before = ai_imc_ld_probe(new1_vaddr, new_sentinel);
    sd(new1_vaddr, new1_payload);
    fence_iorw_iorw();
    new1_after = ai_imc_ld_probe(new1_vaddr, new_sentinel);
    replay_after_new = ai_imc_ld_probe(replay_vaddr, replay_sentinel);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    guard_after = ai_imc_ld_probe(guard_vaddr, 0ULL);
    marker_after = ai_imc_ld_probe(marker_vaddr, 0ULL);
    marker_guard_after = ai_imc_ld_probe(marker_guard_vaddr, 0ULL);
    const bool new_requests_after_repair_make_progress =
        !excpt.triggered &&
        new0_before == new0_seed &&
        new0_after == new0_payload &&
        new1_before == new1_seed &&
        new1_after == new1_payload &&
        replay_after_new == replay_seed &&
        guard_after == guard_seed &&
        marker_after == marker_value &&
        marker_guard_after == marker_guard_seed &&
        new0_payload != replay_seed &&
        new1_payload != replay_seed &&
        new1_payload != ai_imc_handler_log_page[1];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr = 0ULL;
    ai_imc_block_copy_pte_value = 0ULL;
    ai_imc_block_copy_restore_mtvec = 0ULL;
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC replay-like load repairs the original entry once while preserving the pending STIP",
        replay_entry_repaired_with_pending_interrupt,
        "trap_count=%lx cause=%lx tval=%lx expected=%lx mip=%lx repairs=%lx pte=%lx replay=%lx",
        fault_trap_count, fault_cause, fault_tval, replay_vaddr,
        fault_mip, ai_imc_block_copy_repairs, pte_after_repair,
        replay_value);
    TEST_ASSERT("STIP after the replay repair is delivered once and source state is cleared before post-return NC requests",
        interrupt_after_replay_repair_once,
        "rounds=%lx trap_count=%lx causes=%lx,%lx mip1=%lx log1=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_mip[1],
        ai_imc_handler_log_page[1], CSRR(CSR_MIP));
    TEST_ASSERT("two fresh PBMT=NC requests after interrupt return complete without stale replay data, duplicate result, or handler-log payload reuse",
        new_requests_after_repair_make_progress,
        "new0=%lx/%lx new1=%lx/%lx replay=%lx guard=%lx marker=%lx marker_guard=%lx log1=%lx",
        new0_before, new0_after, new1_before, new1_after,
        replay_after_new, guard_after, marker_after,
        marker_guard_after, ai_imc_handler_log_page[1]);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_replay_needfree_progress_smoke_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_store_snapshot_cache_marker_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t offset = 0xe00U;
    const uintptr_t old_vaddr = hs_page_base(nc_page) + offset;
    const uintptr_t new_vaddr = old_vaddr + sizeof(uint64_t);
    const uintptr_t guard_vaddr = old_vaddr + 2U * sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uint64_t old_seed = 0x1122334455667788ULL;
    const uint64_t new_seed = 0x8877665544332211ULL;
    const uint64_t old_expected = 0x11226b7c55665a88ULL;
    const uint64_t new_expected = 0x5e77a1b2c3d42211ULL;
    const uint64_t guard_seed = 0x696d63706e737467ULL;
    const uint64_t marker_value = 0x696d63706e73636dULL;
    const uint64_t marker_guard_seed = 0x696d63706e736367ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t old_initial = 0ULL;
    uint64_t new_initial = 0ULL;
    uint64_t marker_initial = 0ULL;
    uint64_t marker_guard_initial = 0ULL;
    uint64_t old_after_interrupt = 0ULL;
    uint64_t old_final = 0ULL;
    uint64_t new_final = 0ULL;
    uint64_t guard_final = 0ULL;
    uint64_t marker_final = 0ULL;
    uint64_t marker_guard_final = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    sd(old_vaddr, old_seed);
    sd(new_vaddr, new_seed);
    sd(guard_vaddr, guard_seed);
    sd(marker_vaddr, 0ULL);
    sd(marker_guard_vaddr, marker_guard_seed);
    fence_iorw_iorw();
    old_initial = ai_imc_ld_probe(old_vaddr, 0ULL);
    new_initial = ai_imc_ld_probe(new_vaddr, 0ULL);
    marker_initial = ai_imc_ld_probe(marker_vaddr, 0ULL);
    marker_guard_initial = ai_imc_ld_probe(marker_guard_vaddr, 0ULL);
    const bool pbmt_nc_seeded_with_cacheable_marker_clean =
        !excpt.triggered &&
        old_initial == old_seed &&
        new_initial == new_seed &&
        ai_imc_ld_probe(guard_vaddr, 0ULL) == guard_seed &&
        marker_initial == 0ULL &&
        marker_guard_initial == marker_guard_seed;

    TEST_SETUP_EXCEPT();
    sb(old_vaddr + 1U, 0x5aU);
    sh(old_vaddr + 4U, 0x6b7cU);

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    old_after_interrupt = ai_imc_ld_probe(old_vaddr, 0ULL);
    sd(marker_vaddr, marker_value);
    sw(new_vaddr + 2U, 0xa1b2c3d4U);
    sb(new_vaddr + 7U, 0x5eU);
    fence_iorw_iorw();
    old_final = ai_imc_ld_probe(old_vaddr, 0ULL);
    new_final = ai_imc_ld_probe(new_vaddr, 0ULL);
    guard_final = ai_imc_ld_probe(guard_vaddr, 0ULL);
    marker_final = ai_imc_ld_probe(marker_vaddr, 0ULL);
    marker_guard_final = ai_imc_ld_probe(marker_guard_vaddr, 0ULL);
    const bool store_snapshots_and_cache_marker_are_disjoint =
        !excpt.triggered &&
        old_after_interrupt == old_expected &&
        old_final == old_expected &&
        new_final == new_expected &&
        guard_final == guard_seed &&
        marker_final == marker_value &&
        marker_guard_final == marker_guard_seed &&
        old_expected != new_expected &&
        marker_value != old_expected &&
        marker_value != new_expected;

    goto_priv(PRIV_M);
    const bool interrupt_between_nc_store_episodes_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC old/new store targets are seeded while the cacheable marker remains clean",
        pbmt_nc_seeded_with_cacheable_marker_clean,
        "old=%lx new=%lx guard=%lx marker=%lx marker_guard=%lx",
        old_initial, new_initial, guard_seed,
        marker_initial, marker_guard_initial);
    TEST_ASSERT("STIP is delivered once between PBMT=NC store episodes and clears pending state before the younger cacheable marker",
        interrupt_between_nc_store_episodes_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("old and new PBMT=NC partial stores keep separate addr/data/mask snapshots and do not pollute the cacheable marker",
        store_snapshots_and_cache_marker_are_disjoint,
        "old_after_irq=%lx old_final=%lx new_final=%lx guard=%lx marker=%lx marker_guard=%lx log=%lx",
        old_after_interrupt, old_final, new_final, guard_final,
        marker_final, marker_guard_final, ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_store_snapshot_cache_marker_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_cache_alias_oracle_corner()
{
    TEST_START();

    const enum test_page nc_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page cache_alias_page = SWITCH2;
    const enum test_page log_page = SCRATCHPAD;
    const uintptr_t offset = 0xd80U;
    const uintptr_t nc_vaddr = hs_page_base(nc_page) + offset;
    const uintptr_t cache_vaddr = hs_page_base(cache_alias_page) + offset;
    const uintptr_t backing_paddr = phys_page_base(nc_page) + offset;
    const uintptr_t nc_guard_vaddr = nc_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(log_page) + offset;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uint64_t nc_seed = 0x696d63706e63616eULL;
    const uint64_t cache_payload = 0x696d637063616368ULL;
    const uint64_t nc_final = 0x696d63706e636166ULL;
    const uint64_t nc_guard_seed = 0x696d63706e636167ULL;
    const uint64_t marker_value = 0x696d63706e63616dULL;
    const uint64_t marker_guard_seed = 0x696d63706e636177ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t nc_seed_read = 0ULL;
    uint64_t cache_seed_read = 0ULL;
    uint64_t cache_after_store = 0ULL;
    uint64_t nc_after_cache_store = 0ULL;
    uint64_t nc_final_read = 0ULL;
    uint64_t cache_after_nc_final = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    write64(backing_paddr, 0ULL);
    write64(backing_paddr + sizeof(uint64_t), nc_guard_seed);
    fence_iorw_iorw();
    hspt_init();
    ai_imc_write_hs_leaf_to_page(nc_page, nc_page,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1, 1ULL);
    ai_imc_write_hs_leaf_to_page(cache_alias_page, nc_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(log_page, log_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    sd(nc_vaddr, nc_seed);
    sd(nc_guard_vaddr, nc_guard_seed);
    sd(marker_vaddr, 0ULL);
    sd(marker_guard_vaddr, marker_guard_seed);
    fence_iorw_iorw();
    nc_seed_read = ai_imc_ld_probe(nc_vaddr, 0ULL);
    const bool nc_alias_seeds_and_checks_itself =
        !excpt.triggered &&
        nc_seed_read == nc_seed &&
        ai_imc_ld_probe(nc_guard_vaddr, 0ULL) == nc_guard_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cache_seed_read = ai_imc_ld_probe(cache_vaddr, 0ULL);
    sd(cache_vaddr, cache_payload);
    fence_iorw_iorw();
    cache_after_store = ai_imc_ld_probe(cache_vaddr, 0ULL);
    nc_after_cache_store = ai_imc_ld_probe(nc_vaddr, 0ULL);
    sd(nc_vaddr, nc_final);
    fence_iorw_iorw();
    nc_final_read = ai_imc_ld_probe(nc_vaddr, 0ULL);
    cache_after_nc_final = ai_imc_ld_probe(cache_vaddr, 0ULL);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    const bool cache_alias_observes_and_updates_same_pa =
        !excpt.triggered &&
        cache_seed_read == nc_seed &&
        cache_after_store == cache_payload &&
        nc_after_cache_store == cache_payload &&
        nc_final_read == nc_final &&
        cache_after_nc_final == nc_final;
    const bool no_direct_or_handler_oracle_pollution =
        ai_imc_ld_probe(nc_guard_vaddr, 0ULL) == nc_guard_seed &&
        ai_imc_ld_probe(marker_vaddr, 0ULL) == marker_value &&
        ai_imc_ld_probe(marker_guard_vaddr, 0ULL) == marker_guard_seed &&
        cache_after_nc_final != ai_imc_handler_log_page[0] &&
        nc_final_read != ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC alias seeds and verifies its own side effect before any cacheable alias oracle is used",
        nc_alias_seeds_and_checks_itself,
        "nc_seed_read=%lx seed=%lx guard=%lx",
        nc_seed_read, nc_seed, nc_guard_seed);
    TEST_ASSERT("STIP between PBMT=NC and cacheable alias episodes is logged exactly once and clears pending state",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("cacheable alias and PBMT=NC alias converge only through explicit same-PA alias probes and never through direct-PA readback",
        cache_alias_observes_and_updates_same_pa &&
        no_direct_or_handler_oracle_pollution,
        "cache_seed=%lx cache_after=%lx nc_after_cache=%lx nc_final=%lx cache_final=%lx log=%lx",
        cache_seed_read, cache_after_store, nc_after_cache_store,
        nc_final_read, cache_after_nc_final,
        ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_cache_alias_oracle_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_to_cacheable_retry_toggle_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWXPbmt1_GURWXPbmt1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t offset = 0xdc0U;
    const uintptr_t target_vaddr = hs_page_base(target_page) + offset;
    const uintptr_t guard_vaddr = target_vaddr + sizeof(uint64_t);
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_guard_vaddr = marker_vaddr + sizeof(uint64_t);
    const uint64_t nc_seed = 0x696d63706e63746fULL;
    const uint64_t cacheable_payload = 0x696d637063616374ULL;
    const uint64_t guard_seed = 0x696d63706e637467ULL;
    const uint64_t marker_value = 0x696d63706e63746dULL;
    const uint64_t marker_guard_seed = 0x696d63706e637477ULL;
    const uint64_t nc_leaf =
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    const uint64_t cacheable_leaf =
        PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t nc_readback = 0ULL;
    uint64_t cacheable_first_read = 0ULL;
    uint64_t cacheable_final_read = 0ULL;
    uint64_t leaf_after_nc = 0ULL;
    uint64_t leaf_after_toggle = 0ULL;
    uint64_t guard_after_retry = 0ULL;
    uint64_t marker_after_retry = 0ULL;
    uint64_t marker_guard_after_retry = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(target_page, target_page, nc_leaf, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        cacheable_leaf, 1ULL);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    sd(target_vaddr, nc_seed);
    sd(guard_vaddr, guard_seed);
    sd(marker_vaddr, 0ULL);
    sd(marker_guard_vaddr, marker_guard_seed);
    fence_iorw_iorw();
    nc_readback = ai_imc_ld_probe(target_vaddr, 0ULL);
    const bool nc_generation_self_checked =
        !excpt.triggered &&
        nc_readback == nc_seed &&
        ai_imc_ld_probe(guard_vaddr, 0ULL) == guard_seed;

    goto_priv(PRIV_M);
    leaf_after_nc = *ai_imc_hs_leaf_entry(target_page);
    ai_imc_write_hs_leaf_to_page(target_page, target_page,
        cacheable_leaf, 0ULL);
    leaf_after_toggle = *ai_imc_hs_leaf_entry(target_page);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cacheable_first_read = ai_imc_ld_probe(target_vaddr, 0ULL);
    sd(target_vaddr, cacheable_payload);
    fence_iorw_iorw();
    cacheable_final_read = ai_imc_ld_probe(target_vaddr, 0ULL);
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    guard_after_retry = ai_imc_ld_probe(guard_vaddr, 0ULL);
    marker_after_retry = ai_imc_ld_probe(marker_vaddr, 0ULL);
    marker_guard_after_retry = ai_imc_ld_probe(marker_guard_vaddr, 0ULL);
    const bool cacheable_retry_uses_new_leaf_generation =
        !excpt.triggered &&
        (leaf_after_nc & PTE_Pbmt1) != 0ULL &&
        (leaf_after_toggle & PTE_Pbmt1) == 0ULL &&
        cacheable_first_read == nc_seed &&
        cacheable_final_read == cacheable_payload &&
        guard_after_retry == guard_seed &&
        marker_after_retry == marker_value &&
        marker_guard_after_retry == marker_guard_seed;

    goto_priv(PRIV_M);
    const bool interrupt_between_generations_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("same VA is first written and checked through the PBMT=NC leaf generation",
        nc_generation_self_checked,
        "nc_read=%lx seed=%lx guard=%lx leaf=%lx",
        nc_readback, nc_seed, guard_seed, leaf_after_nc);
    TEST_ASSERT("STIP between PBMT=NC and cacheable leaf generations is delivered once and clears pending state",
        interrupt_between_generations_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("after toggling the same VA leaf to cacheable and executing SFENCE.VMA, retry load/store uses the new generation",
        cacheable_retry_uses_new_leaf_generation,
        "leaf_nc=%lx leaf_new=%lx first=%lx final=%lx marker=%lx guard=%lx marker_guard=%lx",
        leaf_after_nc, leaf_after_toggle, cacheable_first_read,
        cacheable_final_read, marker_after_retry, guard_after_retry,
        marker_guard_after_retry);

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_to_cacheable_retry_toggle_corner);
}

bool ai_interrupt_memory_hs_stip_same_va_remap_store_generation_corner()
{
    TEST_START();

    const enum test_page va_page = SWITCH1;
    const enum test_page new_page = SWITCH2;
    const enum test_page guard_page = SCRATCHPAD;
    const uintptr_t vaddr = hs_page_base(va_page) + 0xd00U;
    const uintptr_t old_paddr = phys_page_base(va_page) + 0xd00U;
    const uintptr_t new_paddr = phys_page_base(new_page) + 0xd00U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard = new_paddr + sizeof(uint64_t);
    const uintptr_t handler_guard = phys_page_base(guard_page) + 0xd20U;
    const uint64_t old_seed = 0x696d6372766f6c64ULL;
    const uint64_t old_payload = 0x696d6372766f7374ULL;
    const uint64_t new_seed = 0x696d6372766e6577ULL;
    const uint64_t new_payload = 0x696d6372766e7374ULL;
    const uint64_t old_guard_seed = 0x696d637276676f6cULL;
    const uint64_t new_guard_seed = 0x696d637276676e65ULL;
    const uint64_t handler_guard_seed = 0x696d637276686c67ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t old_read = 0ULL;
    uint64_t old_after_store = 0ULL;
    uint64_t new_first_read = 0ULL;
    uint64_t new_after_store = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(old_guard, old_guard_seed);
    write64(new_guard, new_guard_seed);
    write64(handler_guard, handler_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(va_page, va_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(new_page, new_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    ai_imc_write_hs_leaf_to_page(guard_page, guard_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    old_read = ai_imc_ld_probe(vaddr, 0ULL);
    sd(vaddr, old_payload);
    old_after_store = ai_imc_ld_probe(vaddr, 0ULL);
    const bool old_generation_committed =
        !excpt.triggered &&
        old_read == old_seed &&
        old_after_store == old_payload;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_write_hs_leaf_to_page(va_page, new_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    new_first_read = ai_imc_ld_probe(vaddr, 0ULL);
    sd(vaddr, new_payload);
    new_after_store = ai_imc_ld_probe(vaddr, 0ULL);
    const bool new_generation_visible =
        !excpt.triggered &&
        new_first_read == new_seed &&
        new_after_store == new_payload;

    goto_priv(PRIV_M);
    const bool interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool old_new_pa_isolated =
        read64(old_paddr) == old_payload &&
        read64(new_paddr) == new_payload &&
        read64(old_guard) == old_guard_seed &&
        read64(new_guard) == new_guard_seed &&
        read64(handler_guard) == handler_guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("before remap, the same VA store commits only to the old physical generation",
        old_generation_committed,
        "old_read=%lx old_after=%lx seed=%lx payload=%lx",
        old_read, old_after_store, old_seed, old_payload);
    TEST_ASSERT("STIP in the same-VA remap window is logged exactly once and clears before the new-generation retry",
        interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after remap and SFENCE.VMA, the same VA reads and writes only the new physical generation",
        new_generation_visible && old_new_pa_isolated,
        "new_first=%lx new_after=%lx old_pa=%lx new_pa=%lx guards=%lx,%lx handler_guard=%lx",
        new_first_read, new_after_store, read64(old_paddr),
        read64(new_paddr), read64(old_guard), read64(new_guard),
        read64(handler_guard));

    TEST_END(ai_interrupt_memory_hs_stip_same_va_remap_store_generation_corner);
}

bool ai_interrupt_memory_m_stip_store_fault_same_pc_retry_once_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0xd80U;
    const uintptr_t paddr = phys_page_base(target_page) + 0xd80U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t marker = paddr + 2U * sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d637265747279ULL;
    const uint64_t payload = 0x696d63726574724fULL;
    const uint64_t guard_seed = 0x696d637265746775ULL;
    const uint64_t marker_value = 0x696d637265746d6bULL;
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
    uint64_t after_store = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t fault_trap_count = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_epc = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_mip = 0ULL;
    uint64_t fault_repairs = 0ULL;
    uintptr_t fault_store_pc = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    write64(marker, 0ULL);
    fence_iorw_iorw();

    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);
    ai_imc_retry_pmpaddr0 = pmp_start >> 2U;
    ai_imc_retry_pmpaddr1 = pmp_end >> 2U;
    ai_imc_retry_pmpaddr2 = (uint64_t)-1;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_retry_store_handler);

    ai_imc_mprv_store_retry_probe(vaddr, payload);
    after_store = read64(paddr);
    fault_trap_count = ai_imc_trap_count;
    fault_cause = ai_imc_trap_cause[0];
    fault_epc = ai_imc_trap_epc[0];
    fault_tval = ai_imc_trap_tval[0];
    fault_mip = CSRR(CSR_MIP);
    fault_repairs = ai_imc_retry_fault_repairs;
    fault_store_pc = ai_imc_retry_store_pc;
    const bool same_pc_retry_committed_once =
        fault_trap_count == 1ULL &&
        fault_cause == CAUSE_SAF &&
        fault_epc == fault_store_pc &&
        fault_tval == vaddr &&
        fault_repairs == 1ULL &&
        after_store == payload &&
        read64(guard) == guard_seed &&
        read64(marker) == 0ULL &&
        (fault_mip & AI_IMC_MIP_STIP) != 0ULL;

    write64(marker, marker_value);
    ai_imc_reset_trap_log();
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    const bool preserved_interrupt_delivered_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        read64(paddr) == payload &&
        read64(guard) == guard_seed &&
        read64(marker) == marker_value;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("faulting MPRV-U store is repaired in mtvec and mret retries the same store PC exactly once",
        same_pc_retry_committed_once,
        "trap_count=%lx cause=%lx epc=%lx store_pc=%lx tval=%lx vaddr=%lx repairs=%lx target=%lx guard=%lx marker=%lx mip=%lx",
        fault_trap_count, fault_cause, fault_epc, fault_store_pc,
        fault_tval, vaddr, fault_repairs, after_store, read64(guard),
        read64(marker), fault_mip);
    TEST_ASSERT("the STIP preserved across the same-PC store retry is later delivered once without duplicating the store side effect",
        preserved_interrupt_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx target=%lx guard=%lx marker=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], read64(paddr), read64(guard),
        read64(marker));

    TEST_END(ai_interrupt_memory_m_stip_store_fault_same_pc_retry_once_corner);
}

bool ai_interrupt_memory_m_stip_fault_repair_log_ring_sequence_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x1e20U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x1e20U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t marker0 = paddr + 2U * sizeof(uint64_t);
    const uintptr_t marker1 = paddr + 3U * sizeof(uint64_t);
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t seed = 0x696d63726c6f6701ULL;
    const uint64_t payload = 0x696d63726c6f6755ULL;
    const uint64_t guard_seed = 0x696d63726c67aa01ULL;
    const uint64_t marker0_value = 0x696d63726c6d6b30ULL;
    const uint64_t marker1_value = 0x696d63726c6d6b31ULL;
    const uint64_t log_magic0 = 0x696d63686c6f6700ULL;
    const uint64_t log_magic1 = 0x696d63686c6f6701ULL;
    const uint64_t log_magic2 = 0x696d63686c6f6702ULL;
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
    uint64_t after_store = 0ULL;
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;
    uintptr_t fault_store_pc = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    write64(marker0, 0ULL);
    write64(marker1, 0ULL);
    fence_iorw_iorw();

    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);
    ai_imc_retry_pmpaddr0 = pmp_start >> 2U;
    ai_imc_retry_pmpaddr1 = pmp_end >> 2U;
    ai_imc_retry_pmpaddr2 = (uint64_t)-1;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_retry_store_handler);

    ai_imc_mprv_store_retry_probe(vaddr, payload);
    after_store = read64(paddr);
    fault_store_pc = ai_imc_retry_store_pc;
    const bool fault_repair_log_slot_ok =
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_SAF &&
        ai_imc_trap_epc[0] == fault_store_pc &&
        ai_imc_trap_tval[0] == vaddr &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == log_magic0 &&
        ai_imc_retry_fault_repairs == 1ULL &&
        after_store == payload &&
        read64(guard) == guard_seed &&
        read64(marker0) == 0ULL &&
        read64(marker1) == 0ULL;

    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds0 = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    write64(marker0, marker0_value);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds1 = ai_imc_spin_until_log_count(3ULL, AI_IMC_SPIN_LIMIT);
    write64(marker1, marker1_value);
    fence_iorw_iorw();

    const bool interrupt_log_slots_are_contiguous =
        rounds0 < AI_IMC_SPIN_LIMIT &&
        rounds1 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        ai_imc_trap_cause[2] == CAUSE_STI &&
        ai_imc_trap_epc[1] != 0ULL &&
        ai_imc_trap_epc[2] != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[2] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[1] == log_magic1 &&
        ai_imc_handler_log_page[2] == log_magic2 &&
        ai_imc_handler_log_page[3] == 0ULL &&
        ai_imc_trap_cause[3] == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool fault_repair_and_markers_not_overwritten =
        read64(paddr) == payload &&
        read64(guard) == guard_seed &&
        read64(marker0) == marker0_value &&
        read64(marker1) == marker1_value &&
        ai_imc_retry_fault_repairs == 1ULL;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("fault repair occupies the first log slot once and retries the store without stealing interrupt slots",
        fault_repair_log_slot_ok,
        "trap_count=%lx cause0=%lx epc0=%lx store_pc=%lx tval0=%lx vaddr=%lx mip0=%lx log0=%lx repairs=%lx target=%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_epc[0], fault_store_pc, ai_imc_trap_tval[0],
        vaddr, ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        ai_imc_retry_fault_repairs, read64(paddr));
    TEST_ASSERT("two subsequent STIP handler entries append contiguous log slots and clear their source exactly once",
        interrupt_log_slots_are_contiguous,
        "rounds=%lx,%lx trap_count=%lx causes=%lx,%lx,%lx logs=%lx,%lx,%lx,%lx final_mip=%lx",
        rounds0, rounds1, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_cause[2],
        ai_imc_handler_log_page[0], ai_imc_handler_log_page[1],
        ai_imc_handler_log_page[2], ai_imc_handler_log_page[3],
        CSRR(CSR_MIP));
    TEST_ASSERT("fault repair target, post-interrupt markers, and guard remain stable after the log-ring sequence",
        fault_repair_and_markers_not_overwritten,
        "target=%lx guard=%lx marker0=%lx marker1=%lx repairs=%lx",
        read64(paddr), read64(guard), read64(marker0),
        read64(marker1), ai_imc_retry_fault_repairs);

    TEST_END(ai_interrupt_memory_m_stip_fault_repair_log_ring_sequence_corner);
}

bool ai_interrupt_memory_m_stip_wrong_path_store_handler_load_no_leak_corner()
{
    TEST_START();

    const uintptr_t target = phys_page_base(SCRATCHPAD) + 0x1f00U;
    const uintptr_t guard = target + sizeof(uint64_t);
    const uintptr_t marker0 = target + 2U * sizeof(uint64_t);
    const uintptr_t marker1 = target + 3U * sizeof(uint64_t);
    const uint64_t seed = 0x696d637770617468ULL;
    const uint64_t skipped_payload = 0x696d637777726f6eULL;
    const uint64_t committed_payload = 0x696d6377636f6d6dULL;
    const uint64_t guard_seed = 0x696d63777067aa01ULL;
    const uint64_t marker0_value = 0x696d6377706d6b30ULL;
    const uint64_t marker1_value = 0x696d6377706d6b31ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;
    uint64_t target_after_skipped_store = 0ULL;
    uint64_t mip_after_skipped_store = 0ULL;

    goto_priv(PRIV_M);
    write64(target, seed);
    write64(guard, guard_seed);
    write64(marker0, 0ULL);
    write64(marker1, 0ULL);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = target;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 1\n\t"
        "bnez t0, 1f\n\t"
        "sd %[skipped], 0(%[target])\n\t"
        "1:\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        ".option pop\n\t"
        :
        : [target] "r"(target),
          [skipped] "r"(skipped_payload),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "t0", "memory");
    rounds0 = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    target_after_skipped_store = read64(target);
    mip_after_skipped_store = CSRR(CSR_MIP);
    write64(marker0, marker0_value);
    fence_iorw_iorw();

    const bool skipped_store_not_visible_to_handler =
        rounds0 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == seed &&
        target_after_skipped_store == seed &&
        target_after_skipped_store != skipped_payload &&
        (mip_after_skipped_store & AI_IMC_MIP_STIP) == 0ULL;

    write64(target, committed_payload);
    fence_iorw_iorw();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds1 = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    write64(marker1, marker1_value);
    fence_iorw_iorw();

    const bool committed_store_visible_to_handler =
        rounds1 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[1] == committed_payload &&
        read64(target) == committed_payload &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool logs_markers_and_guard_isolated =
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        ai_imc_handler_log_page[2] == 0ULL &&
        read64(marker0) == marker0_value &&
        read64(marker1) == marker1_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("branch-skipped store payload is not visible to the interrupt handler same-address load or final target image",
        skipped_store_not_visible_to_handler,
        "rounds=%lx trap_count=%lx cause0=%lx probe0=%lx target_after=%lx skipped=%lx mip=%lx",
        rounds0, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], target_after_skipped_store,
        skipped_payload, mip_after_skipped_store);
    TEST_ASSERT("committed control store is visible to the second interrupt handler load and correct-path final read",
        committed_store_visible_to_handler,
        "rounds=%lx trap_count=%lx cause1=%lx probe1=%lx target=%lx committed=%lx final_mip=%lx",
        rounds1, ai_imc_trap_count, ai_imc_trap_cause[1],
        ai_imc_handler_probe_value[1], read64(target),
        committed_payload, CSRR(CSR_MIP));
    TEST_ASSERT("wrong-path smoke, committed control, handler logs, markers and guard stay in separate memory slots",
        logs_markers_and_guard_isolated,
        "logs=%lx,%lx,%lx marker0=%lx marker1=%lx guard=%lx",
        ai_imc_handler_log_page[0], ai_imc_handler_log_page[1],
        ai_imc_handler_log_page[2], read64(marker0),
        read64(marker1), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_wrong_path_store_handler_load_no_leak_corner);
}

bool ai_interrupt_memory_hs_stip_fault_handler_block_copy_retry_owner_corner()
{
    TEST_START();

    const enum test_page target_page = VSR_GX;
    const enum test_page template_page = SCRATCHPAD;
    const enum test_page marker_page = SWITCH1;
    const uintptr_t target_vaddr = hs_page_base(target_page) + 0xe00U;
    const uintptr_t target_paddr = phys_page_base(target_page) + 0xe00U;
    const uintptr_t template_paddr = phys_page_base(template_page) + 0xe00U;
    const uintptr_t target_guard = target_paddr + 4U * sizeof(uint64_t);
    const uintptr_t template_guard =
        template_paddr + 4U * sizeof(uint64_t);
    const uintptr_t marker = phys_page_base(marker_page) + 0xe00U;
    const uintptr_t marker_guard = marker + sizeof(uint64_t);
    const uint64_t target_seed_base = 0x696d636263746700ULL;
    const uint64_t template_base = 0x696d636263746d00ULL;
    const uint64_t target_guard_seed = 0x696d636263746775ULL;
    const uint64_t template_guard_seed = 0x696d636263746775ULL;
    const uint64_t marker_value = 0x696d636263746d6bULL;
    const uint64_t marker_guard_seed = 0x696d636263746768ULL;
    const uint64_t no_read_leaf = PTE_V | PTE_AD | PTE_X;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t repaired_load = 0ULL;
    uint64_t post_retry_second = 0ULL;
    uint64_t fault_trap_count = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_epc = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_mip = 0ULL;
    uint64_t fault_repairs = 0ULL;
    uint64_t pte_after_repair = 0ULL;
    uint64_t rounds = 0ULL;
    bool copy_image_ok = true;
    bool target_seed_clean = true;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        write64(target_paddr + i * sizeof(uint64_t),
            target_seed_base + i);
        write64(template_paddr + i * sizeof(uint64_t),
            template_base + i);
    }
    write64(target_guard, target_guard_seed);
    write64(template_guard, template_guard_seed);
    write64(marker, 0ULL);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(target_page, no_read_leaf);
    ai_imc_set_hs_leaf(template_page, legal_leaf);
    ai_imc_set_hs_leaf(marker_page, legal_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRC(CSR_MEDELEG, 1ULL << CAUSE_LPF);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = template_paddr;
    ai_imc_block_copy_dst_addr = target_paddr;
    ai_imc_block_copy_words = 4ULL;
    ai_imc_block_copy_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(target_page);
    ai_imc_block_copy_pte_value =
        (phys_page_base(target_page) >> 2U) | legal_leaf;
    ai_imc_block_copy_restore_mtvec = saved_mtvec;
    CSRW(CSR_MTVEC,
        (uintptr_t)ai_imc_mtvec_block_copy_repair_handler);

    goto_priv(PRIV_HS);
    repaired_load = ai_imc_ld_probe(target_vaddr, 0ULL);
    post_retry_second = ld(target_vaddr + sizeof(uint64_t));
    sd(hs_page_base(marker_page) + 0xe00U, marker_value);

    goto_priv(PRIV_M);
    fault_trap_count = ai_imc_trap_count;
    fault_cause = ai_imc_trap_cause[0];
    fault_epc = ai_imc_trap_epc[0];
    fault_tval = ai_imc_trap_tval[0];
    fault_mip = ai_imc_trap_mip[0];
    fault_repairs = ai_imc_block_copy_repairs;
    pte_after_repair = *ai_imc_hs_leaf_entry(target_page);
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        if (read64(target_paddr + i * sizeof(uint64_t)) !=
            template_base + i) {
            copy_image_ok = false;
        }
        if (read64(target_paddr + i * sizeof(uint64_t)) ==
            target_seed_base + i) {
            target_seed_clean = false;
        }
    }
    const bool handler_repaired_and_retried_once =
        fault_trap_count == 1ULL &&
        fault_cause == CAUSE_LPF &&
        fault_tval == target_vaddr &&
        fault_epc != 0ULL &&
        fault_repairs == 1ULL &&
        (fault_mip & AI_IMC_MIP_STIP) != 0ULL &&
        repaired_load == template_base &&
        post_retry_second == template_base + 1ULL &&
        pte_after_repair ==
            ((phys_page_base(target_page) >> 2U) | legal_leaf);
    const bool copy_buffer_and_guards_isolated =
        copy_image_ok &&
        target_seed_clean &&
        read64(template_paddr) == template_base &&
        read64(template_paddr + sizeof(uint64_t)) ==
            template_base + 1ULL &&
        read64(template_guard) == template_guard_seed &&
        read64(target_guard) == target_guard_seed &&
        read64(marker) == marker_value &&
        read64(marker_guard) == marker_guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool preserved_stip_delivered_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        read64(target_paddr) == template_base &&
        read64(marker) == marker_value;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr = 0ULL;
    ai_imc_block_copy_pte_value = 0ULL;
    ai_imc_block_copy_restore_mtvec = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("M trap handler block-copies the template into the fault page, repairs the HS leaf, and mret retries the same load once",
        handler_repaired_and_retried_once,
        "trap_count=%lx cause=%lx epc=%lx tval=%lx expected=%lx repairs=%lx first=%lx second=%lx pte=%lx mip=%lx",
        fault_trap_count, fault_cause, fault_epc, fault_tval,
        target_vaddr, fault_repairs, repaired_load, post_retry_second,
        pte_after_repair, fault_mip);
    TEST_ASSERT("the block-copy repair image, template buffer, post-retry marker, and guards remain separate after retry",
        copy_buffer_and_guards_isolated,
        "target0=%lx target1=%lx template0=%lx template1=%lx tguard=%lx sguard=%lx marker=%lx marker_guard=%lx",
        read64(target_paddr), read64(target_paddr + sizeof(uint64_t)),
        read64(template_paddr), read64(template_paddr + sizeof(uint64_t)),
        read64(target_guard), read64(template_guard), read64(marker),
        read64(marker_guard));
    TEST_ASSERT("the STIP pending across block-copy fault repair is delivered once after the retry and does not rewrite the repaired page",
        preserved_stip_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx target0=%lx marker=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP), read64(target_paddr),
        read64(marker));

    TEST_END(ai_interrupt_memory_hs_stip_fault_handler_block_copy_retry_owner_corner);
}

bool ai_interrupt_memory_hs_ssip_handler_stack_fault_retry_owner_corner()
{
    TEST_START();

    const enum test_page main_page = VSRWX_GRWX;
    const enum test_page spill_low_page = SWITCH1;
    const enum test_page spill_high_page = SWITCH2;
    const uintptr_t main_vaddr = hs_page_base(main_page) + 0x2c0U;
    const uintptr_t main_paddr = phys_page_base(main_page) + 0x2c0U;
    const uintptr_t main_guard = main_paddr + 0x10U;
    const uintptr_t marker = main_paddr + 0x18U;
    const uintptr_t spill_base_vaddr = hs_page_base(spill_low_page) +
        PAGE_SIZE - 16U;
    const uintptr_t spill_low_paddr = phys_page_base(spill_low_page) +
        PAGE_SIZE - 16U;
    const uintptr_t spill_high_paddr = phys_page_base(spill_high_page);
    const uintptr_t spill_low_guard = spill_low_paddr - 8U;
    const uintptr_t spill_high_guard = spill_high_paddr + 32U;
    const uint64_t main_seed = 0x696d6373666d6169ULL;
    const uint64_t main_payload = 0x696d6373666d7079ULL;
    const uint64_t main_guard_seed = 0x696d6373666d6775ULL;
    const uint64_t marker_value = 0x696d6373666d6b72ULL;
    const uint64_t spill_low_guard_seed = 0x696d6373666c6f77ULL;
    const uint64_t spill_high_guard_seed = 0x696d637366686967ULL;
    const uint64_t spill0 = 0x696d637373663031ULL;
    const uint64_t spill1 = 0x696d637373663132ULL;
    const uint64_t spill2 = 0x696d637373663233ULL;
    const uint64_t spill3 = 0x696d637373663334ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    uint64_t main_fault_cause = 0ULL;
    uint64_t main_fault_tval = 0ULL;
    uint64_t main_fault_trap_count = 0ULL;
    uint64_t pending_after_main_fault = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(main_paddr, main_seed);
    write64(main_guard, main_guard_seed);
    write64(marker, 0ULL);
    write64(spill_low_guard, spill_low_guard_seed);
    write64(spill_low_paddr, 0xaaaaaaaaaaaaaaaaULL);
    write64(spill_low_paddr + 8U, 0xbbbbbbbbbbbbbbbbULL);
    write64(spill_high_paddr, 0xccccccccccccccccULL);
    write64(spill_high_paddr + 8U, 0xddddddddddddddddULL);
    write64(spill_high_guard, spill_high_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_stack_fault_spill_base = spill_base_vaddr;
    ai_imc_stack_fault_saved_sp = 0ULL;
    ai_imc_stack_fault_saved_sstatus = 0ULL;
    ai_imc_stack_fault_main_pte_addr = 0ULL;
    ai_imc_stack_fault_main_pte_value = 0ULL;
    ai_imc_stack_fault_spill_pte_addr = 0ULL;
    ai_imc_stack_fault_spill_pte_value = 0ULL;
    ai_imc_stack_fault_in_interrupt = 0ULL;
    ai_imc_stack_fault_main_repairs = 0ULL;
    ai_imc_stack_fault_spill_repairs = 0ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(main_page, PTE_V | PTE_AD | PTE_R);
    ai_imc_set_hs_leaf(spill_low_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_set_hs_leaf(spill_high_page, PTE_V | PTE_AD | PTE_R);
    ai_imc_stack_fault_main_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(main_page);
    ai_imc_stack_fault_main_pte_value =
        (phys_page_base(main_page) >> 2U) |
        (PTE_V | PTE_AD | PTE_RWX);
    ai_imc_stack_fault_spill_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(spill_high_page);
    ai_imc_stack_fault_spill_pte_value =
        (phys_page_base(spill_high_page) >> 2U) |
        (PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRW(CSR_STVEC, (uintptr_t)ai_imc_stvec_stack_fault_repair_handler);
    goto_priv(PRIV_M);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_SPF);
    CSRS(CSR_MIDELEG, AI_IMC_SIE_SSIE);
    goto_priv(PRIV_HS);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    sd(main_vaddr, main_payload);
    main_fault_trap_count = ai_imc_trap_count;
    main_fault_cause = ai_imc_trap_cause[0];
    main_fault_tval = ai_imc_trap_tval[0];
    pending_after_main_fault = CSRR(CSR_SIP);
    const bool main_store_fault_repaired_once =
        main_fault_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_SPF &&
        main_fault_cause == CAUSE_SPF &&
        main_fault_tval == main_vaddr &&
        ai_imc_trap_tval[0] == main_vaddr &&
        ai_imc_stack_fault_main_repairs == 1ULL &&
        read64(main_paddr) == main_payload &&
        read64(main_guard) == main_guard_seed &&
        read64(marker) == 0ULL &&
        (pending_after_main_fault & AI_IMC_SIP_SSIP) == 0ULL;

    TEST_SETUP_EXCEPT();
    ai_imc_stack_fault_in_interrupt = 0ULL;
    CSRS(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRS(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    rounds = ai_imc_spin_until_log_count(3ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool interrupt_stack_fault_contexts_split =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[1] == CAUSE_SSI &&
        ai_imc_trap_cause[2] == CAUSE_SPF &&
        ai_imc_trap_tval[2] == hs_page_base(spill_high_page) &&
        ai_imc_stack_fault_spill_repairs == 1ULL &&
        ai_imc_stack_fault_in_interrupt == 0ULL &&
        (CSRR(CSR_SIP) & AI_IMC_SIP_SSIP) == 0ULL;
    const bool spill_and_main_images_disjoint =
        read64(spill_low_guard) == spill_low_guard_seed &&
        read64(spill_low_paddr) == spill0 &&
        read64(spill_low_paddr + 8U) == spill1 &&
        read64(spill_high_paddr) == spill2 &&
        read64(spill_high_paddr + 8U) == spill3 &&
        read64(spill_high_guard) == spill_high_guard_seed &&
        read64(main_paddr) == main_payload &&
        read64(main_guard) == main_guard_seed &&
        read64(marker) == marker_value;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    ai_imc_stack_fault_spill_base = 0ULL;
    ai_imc_stack_fault_saved_sstatus = 0ULL;
    ai_imc_stack_fault_main_pte_addr = 0ULL;
    ai_imc_stack_fault_spill_pte_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("mainline HS store fault is repaired by the custom stvec exactly once while no SSIP is pending yet",
        main_store_fault_repaired_once,
        "main_trap_count=%lx cause0=%lx main_cause=%lx tval=%lx expected=%lx repairs=%lx sip=%lx main=%lx guard=%lx",
        main_fault_trap_count, ai_imc_trap_cause[0], main_fault_cause,
        main_fault_tval, main_vaddr, ai_imc_stack_fault_main_repairs,
        pending_after_main_fault, read64(main_paddr), read64(main_guard));
    TEST_ASSERT("SSIP handler spill crosses into a no-write stack page, repairs that page, and keeps trap contexts separated from the mainline fault",
        interrupt_stack_fault_contexts_split,
        "rounds=%lx trap_count=%lx causes=%lx,%lx,%lx tvals=%lx,%lx,%lx spill_repairs=%lx sip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_trap_cause[2],
        ai_imc_trap_tval[0], ai_imc_trap_tval[1],
        ai_imc_trap_tval[2], ai_imc_stack_fault_spill_repairs,
        CSRR(CSR_SIP));
    TEST_ASSERT("handler stack spill image, mainline target, marker, and guards are all exactly-once and disjoint after the nested repair path",
        spill_and_main_images_disjoint,
        "spill=%lx,%lx,%lx,%lx guards=%lx,%lx main=%lx main_guard=%lx marker=%lx saved_sp=%lx",
        read64(spill_low_paddr), read64(spill_low_paddr + 8U),
        read64(spill_high_paddr), read64(spill_high_paddr + 8U),
        read64(spill_low_guard), read64(spill_high_guard),
        read64(main_paddr), read64(main_guard), read64(marker),
        ai_imc_stack_fault_saved_sp);

    TEST_END(ai_interrupt_memory_hs_ssip_handler_stack_fault_retry_owner_corner);
}

bool ai_interrupt_memory_hs_ssip_nested_handler_fault_preserves_main_retry_corner()
{
    TEST_START();

    const enum test_page main_page = VSRWX_GRWX;
    const enum test_page handler_page = SCRATCHPAD;
    const enum test_page marker_page = SWITCH1;
    const uintptr_t main_vaddr = hs_page_base(main_page) + 0x360U;
    const uintptr_t main_paddr = phys_page_base(main_page) + 0x360U;
    const uintptr_t main_guard = main_paddr + 0x8U;
    const uintptr_t handler_vaddr = hs_page_base(handler_page) + 0x3a0U;
    const uintptr_t handler_paddr = phys_page_base(handler_page) + 0x3a0U;
    const uintptr_t handler_guard = handler_paddr + 0x8U;
    const uintptr_t marker = phys_page_base(marker_page) + 0x480U;
    const uintptr_t marker_guard = marker + 0x8U;
    const uint64_t main_seed = 0x696d636e66706d73ULL;
    const uint64_t main_payload = 0x696d636e66706d70ULL;
    const uint64_t main_guard_seed = 0x696d636e66706d67ULL;
    const uint64_t handler_seed = 0x696d636e66706873ULL;
    const uint64_t handler_payload = 0x696d636e66706870ULL;
    const uint64_t handler_guard_seed = 0x696d636e66706867ULL;
    const uint64_t marker_value = 0x696d636e66706d6bULL;
    const uint64_t marker_guard_seed = 0x696d636e66706b67ULL;
    const uint64_t log0 = 0x696d636e66700000ULL;
    const uint64_t log1 = 0x696d636e66700001ULL;
    const uint64_t log2 = 0x696d636e66700002ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    uint64_t final_sip = 0ULL;

    goto_priv(PRIV_M);
    write64(main_paddr, main_seed);
    write64(main_guard, main_guard_seed);
    write64(handler_paddr, handler_seed);
    write64(handler_guard, handler_guard_seed);
    write64(marker, 0ULL);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_nested_fault_main_pte_addr = 0ULL;
    ai_imc_nested_fault_main_pte_value = 0ULL;
    ai_imc_nested_fault_handler_pte_addr = 0ULL;
    ai_imc_nested_fault_handler_pte_value = 0ULL;
    ai_imc_nested_fault_handler_store_addr = handler_vaddr;
    ai_imc_nested_fault_handler_store_value = handler_payload;
    ai_imc_nested_fault_in_handler = 0ULL;
    ai_imc_nested_fault_main_repairs = 0ULL;
    ai_imc_nested_fault_handler_repairs = 0ULL;
    ai_imc_nested_fault_main_saved_sstatus = 0ULL;
    ai_imc_nested_fault_ssip_saved_sstatus = 0ULL;

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(main_page, PTE_V | PTE_AD | PTE_R);
    ai_imc_set_hs_leaf(handler_page, PTE_V | PTE_AD | PTE_R);
    ai_imc_set_hs_leaf(marker_page, PTE_V | PTE_AD | PTE_RWX);
    ai_imc_nested_fault_main_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(main_page);
    ai_imc_nested_fault_main_pte_value =
        (phys_page_base(main_page) >> 2U) |
        (PTE_V | PTE_AD | PTE_RWX);
    ai_imc_nested_fault_handler_pte_addr =
        (uintptr_t)ai_imc_hs_leaf_entry(handler_page);
    ai_imc_nested_fault_handler_pte_value =
        (phys_page_base(handler_page) >> 2U) |
        (PTE_V | PTE_AD | PTE_RWX);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_STVEC,
        (uintptr_t)ai_imc_stvec_nested_fault_preserve_retry_handler);
    sfence_vma();

    goto_priv(PRIV_M);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_SPF);
    CSRS(CSR_MIDELEG, AI_IMC_SIE_SSIE);
    goto_priv(PRIV_HS);

    TEST_SETUP_EXCEPT();
    sd(main_vaddr, main_payload);
    final_sip = CSRR(CSR_SIP);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool nested_fault_log_is_ordered =
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[0] == CAUSE_SPF &&
        ai_imc_trap_cause[1] == CAUSE_SSI &&
        ai_imc_trap_cause[2] == CAUSE_SPF &&
        ai_imc_trap_tval[0] == main_vaddr &&
        ai_imc_trap_tval[1] == 0ULL &&
        ai_imc_trap_tval[2] == handler_vaddr &&
        ai_imc_handler_log_page[0] == log0 &&
        ai_imc_handler_log_page[1] == log1 &&
        ai_imc_handler_log_page[2] == log2;
    const bool both_faults_repaired_once_and_source_clear =
        ai_imc_nested_fault_main_repairs == 1ULL &&
        ai_imc_nested_fault_handler_repairs == 1ULL &&
        ai_imc_nested_fault_in_handler == 0ULL &&
        (final_sip & AI_IMC_SIP_SSIP) == 0ULL;
    const bool main_retry_and_handler_fault_store_preserved =
        read64(main_paddr) == main_payload &&
        read64(main_guard) == main_guard_seed &&
        read64(handler_paddr) == handler_payload &&
        read64(handler_guard) == handler_guard_seed &&
        read64(marker) == marker_value &&
        read64(marker_guard) == marker_guard_seed;

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    ai_imc_nested_fault_main_pte_addr = 0ULL;
    ai_imc_nested_fault_handler_pte_addr = 0ULL;
    ai_imc_nested_fault_handler_store_addr = 0ULL;
    ai_imc_nested_fault_main_saved_sstatus = 0ULL;
    ai_imc_nested_fault_ssip_saved_sstatus = 0ULL;
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("mainline store page fault, nested SSIP, and handler store page fault keep ordered cause/tval context",
        nested_fault_log_is_ordered,
        "trap_count=%lx causes=%lx,%lx,%lx tvals=%lx,%lx,%lx logs=%lx,%lx,%lx",
        ai_imc_trap_count, ai_imc_trap_cause[0], ai_imc_trap_cause[1],
        ai_imc_trap_cause[2], ai_imc_trap_tval[0], ai_imc_trap_tval[1],
        ai_imc_trap_tval[2], ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1], ai_imc_handler_log_page[2]);
    TEST_ASSERT("handler internal fault is repaired once, SSIP source is cleared, and mainline retry repair remains pending until after the nested handler returns",
        both_faults_repaired_once_and_source_clear,
        "main_repairs=%lx handler_repairs=%lx in_handler=%lx sip=%lx",
        ai_imc_nested_fault_main_repairs,
        ai_imc_nested_fault_handler_repairs,
        ai_imc_nested_fault_in_handler, final_sip);
    TEST_ASSERT("mainline retry store, handler faulting store, marker, and guards are preserved as disjoint exactly-once side effects",
        main_retry_and_handler_fault_store_preserved,
        "main=%lx main_guard=%lx handler=%lx handler_guard=%lx marker=%lx marker_guard=%lx",
        read64(main_paddr), read64(main_guard), read64(handler_paddr),
        read64(handler_guard), read64(marker), read64(marker_guard));

    TEST_END(ai_interrupt_memory_hs_ssip_nested_handler_fault_preserves_main_retry_corner);
}

bool ai_interrupt_memory_hs_stip_fetch_repair_code_patch_retry_image_corner()
{
    TEST_START();

    typedef uint64_t (*ai_imc_fetch_func_t)(void);

    const enum test_page code_page = VSRWX_GRWX;
    const enum test_page template_page = SCRATCHPAD;
    const enum test_page marker_page = SWITCH2;
    const uintptr_t offset = 0xf40U;
    const uintptr_t code_vaddr = hs_page_base(code_page) + offset;
    const uintptr_t code_paddr = phys_page_base(code_page) + offset;
    const uintptr_t template_paddr =
        phys_page_base(template_page) + offset;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + offset;
    const uintptr_t code_guard = code_paddr + sizeof(uint64_t);
    const uintptr_t template_guard = template_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard = marker_paddr + sizeof(uint64_t);
    const uint64_t old_code = 0x0000806701100513ULL;
    const uint64_t new_code = 0x0000806702b00513ULL;
    const uint64_t code_guard_seed = 0x696d636670676364ULL;
    const uint64_t template_guard_seed = 0x696d63667067746dULL;
    const uint64_t marker_value = 0x696d6366706d6b72ULL;
    const uint64_t marker_guard_seed = 0x696d636670676d6bULL;
    const uint64_t no_exec_leaf = PTE_V | PTE_AD | PTE_RW;
    const uint64_t legal_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    ai_imc_fetch_func_t fetch_func = (ai_imc_fetch_func_t)code_vaddr;
    uint64_t first_fault_triggered = 0ULL;
    uint64_t first_fault_priv = 0ULL;
    uint64_t first_fault_cause = 0ULL;
    uint64_t first_fault_epc = 0ULL;
    uint64_t first_fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t final_value = 0ULL;
    uint64_t final_fetch_triggered = 1ULL;
    uint64_t rounds = 0ULL;
    uint64_t interrupt_cause = 0ULL;
    uint64_t interrupt_mip = 0ULL;
    uint64_t pte_after_repair = 0ULL;
    uint64_t final_mip = 0ULL;

    goto_priv(PRIV_M);
    write64(code_paddr, old_code);
    write64(template_paddr, new_code);
    write64(marker_paddr, 0ULL);
    write64(code_guard, code_guard_seed);
    write64(template_guard, template_guard_seed);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_set_hs_leaf(code_page, no_exec_leaf);
    ai_imc_set_hs_leaf(template_page, legal_leaf);
    ai_imc_set_hs_leaf(marker_page, legal_leaf);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_IPF);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    TEST_EXEC_EXCEPT(code_vaddr);
    first_fault_triggered = excpt.triggered;
    first_fault_priv = excpt.priv;
    first_fault_cause = excpt.cause;
    first_fault_epc = excpt.epc;
    first_fault_tval = excpt.tval;
    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    const bool first_fetch_reports_current_no_x_fault =
        first_fault_triggered &&
        first_fault_priv == PRIV_HS &&
        first_fault_cause == CAUSE_IPF &&
        first_fault_epc == code_vaddr &&
        first_fault_tval == code_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        read64(code_paddr) == old_code &&
        read64(template_paddr) == new_code;

    write64(code_paddr, read64(template_paddr));
    *ai_imc_hs_leaf_entry(code_page) =
        (phys_page_base(code_page) >> 2U) | legal_leaf;
    sfence_vma();
    asm volatile("fence.i" ::: "memory");
    pte_after_repair = *ai_imc_hs_leaf_entry(code_page);
    const bool code_patch_and_permission_repair_done =
        read64(code_paddr) == new_code &&
        read64(code_paddr) != old_code &&
        pte_after_repair ==
            ((phys_page_base(code_page) >> 2U) | legal_leaf) &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) != 0ULL;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    final_value = fetch_func();
    final_fetch_triggered = excpt.triggered;
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();
    goto_priv(PRIV_M);
    interrupt_cause = ai_imc_trap_cause[0];
    interrupt_mip = ai_imc_trap_mip[0];
    final_mip = CSRR(CSR_MIP);
    const bool retried_fetch_sees_only_final_image =
        !final_fetch_triggered &&
        final_value == 0x2bULL &&
        read64(code_paddr) == new_code;
    const bool repaired_code_image_and_template_isolated =
        read64(code_paddr) == new_code &&
        read64(code_paddr) != old_code &&
        read64(template_paddr) == new_code &&
        read64(code_guard) == code_guard_seed &&
        read64(template_guard) == template_guard_seed &&
        read64(marker_paddr) == marker_value &&
        read64(marker_guard) == marker_guard_seed;
    const bool preserved_stip_between_repair_and_retry =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        interrupt_cause == CAUSE_STI &&
        (interrupt_mip & AI_IMC_MIP_STIP) != 0ULL &&
        (final_mip & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_block_copy_src_addr = 0ULL;
    ai_imc_block_copy_dst_addr = 0ULL;
    ai_imc_block_copy_words = 0ULL;
    ai_imc_block_copy_pte_addr = 0ULL;
    ai_imc_block_copy_pte_value = 0ULL;
    ai_imc_block_copy_restore_mtvec = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("initial no-X HS fetch reports an instruction page fault at the current code VA while STIP remains pending",
        first_fetch_reports_current_no_x_fault,
        "triggered=%lx priv=%lx cause=%lx epc=%lx tval=%lx code_va=%lx pending=%lx code=%lx old=%lx template=%lx",
        first_fault_triggered, first_fault_priv, first_fault_cause,
        first_fault_epc, first_fault_tval, code_vaddr,
        pending_after_fault, read64(code_paddr), old_code,
        read64(template_paddr));
    TEST_ASSERT("D-side code patch plus PTE repair and fence.i installs the final executable image before retry",
        code_patch_and_permission_repair_done &&
        repaired_code_image_and_template_isolated,
        "code=%lx old=%lx template=%lx pte=%lx marker=%lx guards=%lx,%lx,%lx",
        read64(code_paddr), old_code, read64(template_paddr),
        pte_after_repair, read64(marker_paddr), read64(code_guard),
        read64(template_guard), read64(marker_guard));
    TEST_ASSERT("the STIP pending across fetch repair is delivered once before retrying the final code image",
        preserved_stip_between_repair_and_retry &&
        retried_fetch_sees_only_final_image,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx log=%lx value=%lx fetch_excpt=%lx",
        rounds, ai_imc_trap_count, interrupt_cause, interrupt_mip,
        final_mip, ai_imc_handler_log_page[0], final_value,
        final_fetch_triggered);

    TEST_END(ai_interrupt_memory_hs_stip_fetch_repair_code_patch_retry_image_corner);
}

bool ai_interrupt_memory_m_stip_mixed_width_post_handler_data_select_corner()
{
    TEST_START();

    const uintptr_t base = phys_page_base(SWITCH1) + 0xe40U;
    const uintptr_t handler_probe = phys_page_base(SWITCH1) + 0xe80U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0xe88U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0xe90U;
    const uint64_t line0 = 0x88776655443380f1ULL;
    const uint64_t line1 = 0x01020384a5b6c7d8ULL;
    const uint64_t handler_seed = 0x696d636473656c48ULL;
    const uint64_t marker_value = 0x696d636473656c4dULL;
    const uint64_t guard_seed = 0x696d636473656c47ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t pre_lb = 0ULL;
    uint64_t pre_lhu = 0ULL;
    uint64_t post_lbu = 0ULL;
    uint64_t post_lh = 0ULL;
    uint64_t post_lw = 0ULL;
    uint64_t post_lwu = 0ULL;
    uint64_t post_ld = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(base, line0);
    write64(base + sizeof(uint64_t), line1);
    write64(handler_probe, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    pre_lb = ai_imc_lb_raw64(base + 1U);
    pre_lhu = ai_imc_lhu_raw64(base + 4U);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    post_lbu = ai_imc_lbu_raw64(base + 1U);
    post_lh = ai_imc_lh_raw64(base + 10U);
    post_lw = ai_imc_lw_raw64(base + 8U);
    post_lwu = ai_imc_lwu_raw64(base + 8U);
    post_ld = ai_imc_ld_probe(base + 8U, 0ULL);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool pre_select_result_ok =
        pre_lb == 0xffffffffffffff80ULL &&
        pre_lhu == 0x6655ULL;
    const bool interrupt_handler_load_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_probe_value[0] != line0 &&
        ai_imc_handler_probe_value[0] != line1 &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL;
    const bool post_select_result_ok =
        post_lbu == 0x80ULL &&
        post_lh == 0xffffffffffffa5b6ULL &&
        post_lw == 0xffffffffa5b6c7d8ULL &&
        post_lwu == 0x00000000a5b6c7d8ULL &&
        post_ld == line1;
    const bool memory_image_stable =
        read64(base) == line0 &&
        read64(base + sizeof(uint64_t)) == line1 &&
        read64(handler_probe) == handler_seed &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("pre-interrupt mixed-width loads use their own offset and sign-extension before the pending STIP is delivered",
        pre_select_result_ok,
        "lb=%lx lhu=%lx line0=%lx",
        pre_lb, pre_lhu, line0);
    TEST_ASSERT("STIP handler performs one independent load from its probe target and does not feed the post-return data-select oracle",
        interrupt_handler_load_isolated,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx expected_probe=%lx mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], handler_seed, CSRR(CSR_MIP));
    TEST_ASSERT("post-handler byte/half/word/doubleword loads use their own offset, width and sign-extension with stable memory image",
        post_select_result_ok && memory_image_stable,
        "lbu=%lx lh=%lx lw=%lx lwu=%lx ld=%lx line0=%lx line1=%lx handler=%lx marker=%lx guard=%lx",
        post_lbu, post_lh, post_lw, post_lwu, post_ld,
        read64(base), read64(base + sizeof(uint64_t)),
        read64(handler_probe), read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_mixed_width_post_handler_data_select_corner);
}

bool ai_interrupt_memory_m_stip_store_fault_handler_store_younger_mask_owner_corner()
{
    TEST_START();

    static const uint8_t younger_seed[16] = {
        0x10U, 0x21U, 0x32U, 0x43U,
        0x54U, 0x65U, 0x76U, 0x87U,
        0x98U, 0xa9U, 0xbaU, 0xcbU,
        0xdcU, 0xedU, 0xfeU, 0x0fU,
    };
    static const uint8_t younger_expected[16] = {
        0x10U, 0x91U, 0x92U, 0x43U,
        0x44U, 0x33U, 0x22U, 0x11U,
        0x98U, 0xa9U, 0x66U, 0x55U,
        0xdcU, 0xedU, 0xfeU, 0x0fU,
    };
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page young_page = SWITCH1;
    const enum test_page handler_page = SCRATCHPAD;
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0x1f00U;
    const uintptr_t old_paddr = phys_page_base(old_page) + 0x1f00U;
    const uintptr_t younger_vaddr = hs_page_base(young_page) + 0x1f20U;
    const uintptr_t younger_paddr = phys_page_base(young_page) + 0x1f20U;
    const uintptr_t handler_store = phys_page_base(handler_page) + 0x1f50U;
    const uintptr_t handler_guard = phys_page_base(handler_page) + 0x1f58U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t pmp_start = old_paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t old_seed = 0x696d637371666f6cULL;
    const uint64_t old_denied = 0x696d637371666fddULL;
    const uint64_t old_guard_seed = 0x696d637371666fa1ULL;
    const uint64_t handler_seed = 0x696d637371666850ULL;
    const uint64_t handler_guard_seed = 0x696d6373716668aaULL;
    const uint64_t handler_byte = 0xe7ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t fault_priv = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t old_after_fault = 0ULL;
    uint64_t old_guard_after_fault = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t younger_bad_index = 0ULL;

    goto_priv(PRIV_M);
    write64(old_paddr, old_seed);
    write64(old_guard, old_guard_seed);
    ai_imc_write_bytes(younger_paddr, younger_seed,
        sizeof(younger_seed));
    write64(handler_store, handler_seed);
    write64(handler_guard, handler_guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(old_vaddr, old_denied);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    fault_priv = excpt.priv;
    pending_after_fault = CSRR(CSR_MIP);
    old_after_fault = read64(old_paddr);
    old_guard_after_fault = read64(old_guard);

    ai_imc_handler_store_addr = handler_store;
    ai_imc_handler_store_value = handler_byte;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sb(younger_vaddr + 1U, 0x91U);
    sb(younger_vaddr + 2U, 0x92U);
    sw(younger_vaddr + 4U, 0x11223344U);
    sh(younger_vaddr + 10U, 0x5566U);
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();
    const bool younger_mask_image_ok =
        !excpt.triggered &&
        ai_imc_bytes_match_array(younger_paddr, younger_expected,
            sizeof(younger_expected), &younger_bad_index);
    const bool old_fault_isolated =
        fault_priv == PRIV_M &&
        fault_cause == CAUSE_SAF &&
        fault_tval == old_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        old_after_fault == old_seed &&
        old_guard_after_fault == old_guard_seed &&
        read64(old_paddr) == old_seed &&
        read64(old_guard) == old_guard_seed;
    const bool handler_store_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read8(handler_store) == handler_byte &&
        read64(handler_store) ==
            ((handler_seed & ~0xffULL) | handler_byte) &&
        read64(handler_guard) == handler_guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    ai_imc_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("older PMP-denied MPRV-U store reports one store access fault and leaves its target image unchanged while STIP stays pending",
        old_fault_isolated,
        "priv=%lx cause=%lx tval=%lx pending=%lx old=%lx guard=%lx",
        fault_priv, fault_cause, fault_tval, pending_after_fault,
        old_after_fault, old_guard_after_fault);
    TEST_ASSERT("STIP handler performs exactly one independent byte store and clears the pending source without touching old or younger targets",
        handler_store_isolated,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx handler0=%x handler_word=%lx guard=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], read8(handler_store),
        read64(handler_store), read64(handler_guard),
        CSRR(CSR_MIP));
    TEST_ASSERT("younger post-handler byte/half/word stores use their own data and masks instead of inheriting old fault or handler store metadata",
        younger_mask_image_ok,
        "bad_index=%lx bytes=%x_%x_%x_%x_%x_%x_%x_%x old=%lx handler=%x",
        younger_bad_index, read8(younger_paddr), read8(younger_paddr + 1U),
        read8(younger_paddr + 2U), read8(younger_paddr + 4U),
        read8(younger_paddr + 7U), read8(younger_paddr + 10U),
        read8(younger_paddr + 11U), read8(younger_paddr + 15U),
        read64(old_paddr), read8(handler_store));

    TEST_END(ai_interrupt_memory_m_stip_store_fault_handler_store_younger_mask_owner_corner);
}

bool ai_interrupt_memory_m_stip_amo_queue_pop_handler_log_owner_corner()
{
    TEST_START();

    const uintptr_t head_addr = phys_page_base(SWITCH2) + 0x1f80U;
    const uintptr_t tail_addr = phys_page_base(SWITCH2) + 0x1f88U;
    const uintptr_t entry0_addr = phys_page_base(SWITCH2) + 0x1f90U;
    const uintptr_t entry1_addr = phys_page_base(SWITCH2) + 0x1f98U;
    const uintptr_t handler_marker = phys_page_base(SWITCH2) + 0x1fa0U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x1fa8U;
    const uint64_t tail_seed = 2ULL;
    const uint64_t entry0_payload = 0x696d637171703000ULL;
    const uint64_t entry1_payload = 0x696d637171703111ULL;
    const uint64_t handler_seed = 0x696d637171686d55ULL;
    const uint64_t handler_byte = 0xe9ULL;
    const uint64_t handler_expected =
        (handler_seed & ~0xffULL) | handler_byte;
    const uint64_t guard_seed = 0x696d637171677561ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t first_head = 0ULL;
    uint64_t second_head = 0ULL;
    uint64_t first_payload = 0ULL;
    uint64_t second_payload = 0ULL;
    uint64_t head_after_first = 0ULL;
    uint64_t tail_after_handler = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(head_addr, 0ULL);
    write64(tail_addr, tail_seed);
    write64(entry0_addr, entry0_payload);
    write64(entry1_addr, entry1_payload);
    write64(handler_marker, handler_seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_store_addr = handler_marker;
    ai_imc_handler_store_value = handler_byte;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    first_head = ai_imc_amoadd_d_aqrl((uint64_t *)head_addr, 1ULL);
    head_after_first = read64(head_addr);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    tail_after_handler = read64(tail_addr);

    first_payload = read64(entry0_addr + first_head * sizeof(uint64_t));
    second_head = ai_imc_amoadd_d_aqrl((uint64_t *)head_addr, 1ULL);
    second_payload = read64(entry0_addr + second_head * sizeof(uint64_t));
    fence_iorw_iorw();

    const bool queue_metadata_pop_sequence_ok =
        first_head == 0ULL &&
        head_after_first == 1ULL &&
        second_head == 1ULL &&
        read64(head_addr) == 2ULL &&
        tail_after_handler == tail_seed &&
        read64(tail_addr) == tail_seed;
    const bool payload_sequence_ok =
        first_payload == entry0_payload &&
        second_payload == entry1_payload &&
        first_payload != read64(handler_marker) &&
        second_payload != read64(handler_marker) &&
        read64(entry0_addr) == entry0_payload &&
        read64(entry1_addr) == entry1_payload;
    const bool handler_log_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(handler_marker) == handler_expected &&
        read64(handler_marker) != entry0_payload &&
        read64(handler_marker) != entry1_payload &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("AMO.AQRL queue pop advances head exactly once per pop while the interrupt handler keeps tail metadata untouched",
        queue_metadata_pop_sequence_ok,
        "first_head=%lx head_after_first=%lx second_head=%lx head_final=%lx tail_after=%lx tail_final=%lx",
        first_head, head_after_first, second_head, read64(head_addr),
        tail_after_handler, read64(tail_addr));
    TEST_ASSERT("queue payload loads use entry payloads selected by the AMO old head values and never consume the handler marker",
        payload_sequence_ok,
        "payload0=%lx payload1=%lx entry0=%lx entry1=%lx handler=%lx",
        first_payload, second_payload, read64(entry0_addr),
        read64(entry1_addr), read64(handler_marker));
    TEST_ASSERT("STIP handler writes exactly one independent release-style marker/log and clears source state without joining the queue oracle",
        handler_log_isolated,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx handler=%lx log=%lx guard=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], read64(handler_marker),
        ai_imc_handler_log_page[0], read64(guard), CSRR(CSR_MIP));

    TEST_END(ai_interrupt_memory_m_stip_amo_queue_pop_handler_log_owner_corner);
}

bool ai_interrupt_memory_m_stip_handler_cbozero_main_store_image_corner()
{
    TEST_START();

    const uintptr_t main_line = phys_page_base(SWITCH1) + 0x1e00U;
    const uintptr_t main_before_guard = main_line - sizeof(uint64_t);
    const uintptr_t main_after_guard = main_line + 64U;
    const uintptr_t handler_line = phys_page_base(SWITCH2) + 0x1e00U;
    const uintptr_t handler_before_guard = handler_line - sizeof(uint64_t);
    const uintptr_t handler_after_guard = handler_line + 64U;
    const uint64_t main_before_seed = 0x696d63686d736701ULL;
    const uint64_t main_after_seed = 0x696d63686d736702ULL;
    const uint64_t handler_before_seed = 0x696d636863626701ULL;
    const uint64_t handler_after_seed = 0x696d636863626702ULL;
    const uint64_t log_magic = 0x696d636863626f00ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t main_bad_index = 64ULL;
    bool main_line_ok = true;
    bool handler_line_zeroed = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("handler-CBO main and handler lines are independent 64-byte aligned lines",
        (main_line & 63ULL) == 0ULL &&
        (handler_line & 63ULL) == 0ULL &&
        main_line != handler_line,
        "main=%lx handler=%lx", main_line, handler_line);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        write8(main_line + i, (uint8_t)(0xa0U + i));
        write8(handler_line + i, (uint8_t)(0x40U + i));
    }
    write64(main_before_guard, main_before_seed);
    write64(main_after_guard, main_after_seed);
    write64(handler_before_guard, handler_before_seed);
    write64(handler_after_guard, handler_after_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_cbo_addr = handler_line;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_cbozero_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    sb(main_line + 3U, 0xe1U);
    sh(main_line + 10U, 0xf2d3U);
    sw(main_line + 20U, 0x88776655U);
    sd(main_line + 40U, 0x1122334455667788ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        uint8_t expected = (uint8_t)(0xa0U + i);
        if (i == 3ULL) {
            expected = 0xe1U;
        } else if (i == 10ULL) {
            expected = 0xd3U;
        } else if (i == 11ULL) {
            expected = 0xf2U;
        } else if (i == 20ULL) {
            expected = 0x55U;
        } else if (i == 21ULL) {
            expected = 0x66U;
        } else if (i == 22ULL) {
            expected = 0x77U;
        } else if (i == 23ULL) {
            expected = 0x88U;
        } else if (i == 40ULL) {
            expected = 0x88U;
        } else if (i == 41ULL) {
            expected = 0x77U;
        } else if (i == 42ULL) {
            expected = 0x66U;
        } else if (i == 43ULL) {
            expected = 0x55U;
        } else if (i == 44ULL) {
            expected = 0x44U;
        } else if (i == 45ULL) {
            expected = 0x33U;
        } else if (i == 46ULL) {
            expected = 0x22U;
        } else if (i == 47ULL) {
            expected = 0x11U;
        }
        if (read8(main_line + i) != expected) {
            main_bad_index = i;
            main_line_ok = false;
            break;
        }
    }
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        if (read64(handler_line + i * sizeof(uint64_t)) != 0ULL) {
            handler_line_zeroed = false;
        }
    }

    const bool interrupt_handler_cbo_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == log_magic &&
        ai_imc_handler_log_page[1] == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool main_store_and_handler_cbo_images_isolated =
        main_line_ok &&
        handler_line_zeroed &&
        read64(main_before_guard) == main_before_seed &&
        read64(main_after_guard) == main_after_seed &&
        read64(handler_before_guard) == handler_before_seed &&
        read64(handler_after_guard) == handler_after_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_cbo_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler executes one independent CBO.ZERO line operation and clears the interrupt source",
        interrupt_handler_cbo_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log0=%lx log1=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1], CSRR(CSR_MIP));
    TEST_ASSERT("handler CBO.ZERO line image does not overwrite or merge with the mainline partial-store image",
        main_store_and_handler_cbo_images_isolated,
        "bad_index=%lx main0=%lx main2=%lx main5=%lx handler0=%lx handler7=%lx guards=%lx,%lx,%lx,%lx",
        main_bad_index, read64(main_line), read64(main_line + 16U),
        read64(main_line + 40U), read64(handler_line),
        read64(handler_line + 7U * sizeof(uint64_t)),
        read64(main_before_guard), read64(main_after_guard),
        read64(handler_before_guard), read64(handler_after_guard));

    TEST_END(ai_interrupt_memory_m_stip_handler_cbozero_main_store_image_corner);
}

bool ai_interrupt_memory_m_stip_handler_cbozero_return_first_memory_corner()
{
    TEST_START();

    const uintptr_t main_word = phys_page_base(SCRATCHPAD) + 0x1e40U;
    const uintptr_t post_word = main_word + sizeof(uint64_t);
    const uintptr_t guard_word = main_word + 2U * sizeof(uint64_t);
    const uintptr_t handler_line = phys_page_base(SWITCH2) + 0x1e80U;
    const uintptr_t handler_guard = handler_line + 64U;
    const uint64_t main_seed = 0x696d63686272646dULL;
    const uint64_t main_payload = 0x696d636862726470ULL;
    const uint64_t post_seed = 0x696d636862706f73ULL;
    const uint64_t post_payload = 0x696d636862706f50ULL;
    const uint64_t guard_seed = 0x696d636862677561ULL;
    const uint64_t handler_guard_seed = 0x696d636862686775ULL;
    const uint64_t log_magic = 0x696d636863626f00ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t first_load_after_handler = 0ULL;
    bool handler_line_zeroed = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("post-return handler-CBO target line is 64-byte aligned and separate from scalar words",
        (handler_line & 63ULL) == 0ULL &&
        handler_line != (main_word & ~63ULL),
        "main=%lx handler=%lx", main_word, handler_line);

    CSRS(CSR_MENVCFG, MENVCFG_CBZE);
    write64(main_word, main_seed);
    write64(post_word, post_seed);
    write64(guard_word, guard_seed);
    write64(handler_guard, handler_guard_seed);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(handler_line + i * sizeof(uint64_t),
            0x68646c696e650000ULL + i);
    }
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_cbo_addr = handler_line;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_cbozero_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(main_word, main_payload);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    first_load_after_handler = read64(main_word);
    write64(post_word, post_payload);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        if (read64(handler_line + i * sizeof(uint64_t)) != 0ULL) {
            handler_line_zeroed = false;
        }
    }
    const bool handler_cbo_and_source_contract =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == log_magic &&
        handler_line_zeroed &&
        read64(handler_guard) == handler_guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool post_return_memory_is_main_episode =
        first_load_after_handler == main_payload &&
        read64(main_word) == main_payload &&
        read64(post_word) == post_payload &&
        read64(guard_word) == guard_seed &&
        post_payload != log_magic &&
        post_payload != read64(handler_line);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_cbo_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("handler CBO.ZERO plus fence completes in one STIP handler entry and leaves source cleared before post-return memory",
        handler_cbo_and_source_contract,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx handler0=%lx handler7=%lx guard=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        read64(handler_line), read64(handler_line + 7U * sizeof(uint64_t)),
        read64(handler_guard), CSRR(CSR_MIP));
    TEST_ASSERT("first checked mainline load and post-return store after the handler use mainline data, not handler CBO drain metadata",
        post_return_memory_is_main_episode,
        "first_load=%lx main=%lx post=%lx guard=%lx handler0=%lx",
        first_load_after_handler, read64(main_word), read64(post_word),
        read64(guard_word), read64(handler_line));

    TEST_END(ai_interrupt_memory_m_stip_handler_cbozero_return_first_memory_corner);
}

bool ai_interrupt_memory_m_stip_cboinval_same_line_handler_store_profile_contract_corner()
{
    TEST_START();

    const uintptr_t line = phys_page_base(SWITCH1) + 0x1dc0U;
    const uintptr_t before_guard = line - sizeof(uint64_t);
    const uintptr_t after_guard = line + 64U;
    const uintptr_t marker = line + 80U;
    const uintptr_t handler_lane = line + 45U;
    const uintptr_t post_lane = line + 52U;
    const uint64_t before_seed = 0x696d636976616701ULL;
    const uint64_t after_seed = 0x696d636976616702ULL;
    const uint64_t marker_seed = 0x696d636976616d00ULL;
    const uint64_t marker_value = 0x696d636976616d55ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t bad_index = 64ULL;
    bool line_image_ok = true;
    bool cbo_retired_without_exception = false;

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO.INVAL same-line interrupt target is one aligned cacheable line with external guards",
        (line & 63ULL) == 0ULL &&
        handler_lane < line + 64U &&
        post_lane < line + 64U,
        "line=%lx handler=%lx post=%lx", line, handler_lane,
        post_lane);

    CSRW(CSR_MENVCFG, (saved_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        write8(line + i, (uint8_t)(0x30U + i));
    }
    write64(before_guard, before_seed);
    write64(after_guard, after_seed);
    write64(marker, marker_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_store_addr = handler_lane;
    ai_imc_handler_store_value = 0x7aULL;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    sw(line + 4U, 0xa1b2c3d4U);
    sh(line + 14U, 0xe5f6U);
    sd(line + 24U, 0x1020304050607080ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    TEST_SETUP_EXCEPT();
    cbo_inval(line + 16U);
    fence_iorw_iorw();
    cbo_retired_without_exception = !excpt.triggered;
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sb(post_lane, 0x99U);
    write64(marker, marker_value);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        uint8_t expected = (uint8_t)(0x30U + i);

        if (i == 4ULL) {
            expected = 0xd4U;
        } else if (i == 5ULL) {
            expected = 0xc3U;
        } else if (i == 6ULL) {
            expected = 0xb2U;
        } else if (i == 7ULL) {
            expected = 0xa1U;
        } else if (i == 14ULL) {
            expected = 0xf6U;
        } else if (i == 15ULL) {
            expected = 0xe5U;
        } else if (i == 24ULL) {
            expected = 0x80U;
        } else if (i == 25ULL) {
            expected = 0x70U;
        } else if (i == 26ULL) {
            expected = 0x60U;
        } else if (i == 27ULL) {
            expected = 0x50U;
        } else if (i == 28ULL) {
            expected = 0x40U;
        } else if (i == 29ULL) {
            expected = 0x30U;
        } else if (i == 30ULL) {
            expected = 0x20U;
        } else if (i == 31ULL) {
            expected = 0x10U;
        } else if (i == 45ULL) {
            expected = 0x7aU;
        } else if (i == 52ULL) {
            expected = 0x99U;
        }

        if (read8(line + i) != expected) {
            bad_index = i;
            line_image_ok = false;
            break;
        }
    }

    const bool interrupt_handler_same_line_store_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0ULL &&
        read8(handler_lane) == 0x7aU &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool profile_contract_line_image =
        cbo_retired_without_exception &&
        line_image_ok &&
        read8(line + 4U) == 0xd4U &&
        read8(line + 14U) == 0xf6U &&
        read64(line + 24U) == 0x1020304050607080ULL &&
        read8(post_lane) == 0x99U &&
        read64(before_guard) == before_seed &&
        read64(after_guard) == after_seed &&
        read64(marker) == marker_value;
    const bool no_pure_inval_or_handler_owner_leak =
        read8(line + 0U) == 0x30U &&
        read8(line + 5U) == 0xc3U &&
        read8(line + 45U) == 0x7aU &&
        read8(line + 52U) == 0x99U &&
        read8(line + 63U) == 0x6fU &&
        read8(handler_lane) != read8(post_lane) &&
        read64(marker) != ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("CBO.INVAL retires under the profile-selected CBIE=Flush contract while STIP remains pending until enabled",
        cbo_retired_without_exception &&
        interrupt_handler_same_line_store_once,
        "cbo_ok=%d rounds=%lx trap_count=%lx cause=%lx mip0=%lx log0=%lx log1=%lx final_mip=%lx",
        cbo_retired_without_exception, rounds, ai_imc_trap_count,
        ai_imc_trap_cause[0], ai_imc_trap_mip[0],
        ai_imc_handler_log_page[0], ai_imc_handler_log_page[1],
        CSRR(CSR_MIP));
    TEST_ASSERT("dirty mainline bytes, same-line handler byte, post-return byte and guards form one legal final image after CBO.INVAL",
        profile_contract_line_image,
        "bad=%lx line0=%lx line3=%lx line5=%lx handler=%x post=%x before=%lx after=%lx marker=%lx",
        bad_index, read64(line), read64(line + 24U),
        read64(line + 40U), read8(handler_lane), read8(post_lane),
        read64(before_guard), read64(after_guard), read64(marker));
    TEST_ASSERT("the CBO.INVAL oracle is not treated as pure invalidate and the handler byte is not accepted as a whole-line owner",
        no_pure_inval_or_handler_owner_leak,
        "b0=%x b5=%x h=%x p=%x b63=%x marker=%lx log=%lx",
        read8(line), read8(line + 5U), read8(handler_lane),
        read8(post_lane), read8(line + 63U), read64(marker),
        ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_m_stip_cboinval_same_line_handler_store_profile_contract_corner);
}

bool ai_interrupt_memory_m_stip_cbom_clean_flush_handler_read_visibility_corner()
{
    TEST_START();

    const uintptr_t line = phys_page_base(SWITCH2) + 0x1d40U;
    const uintptr_t target = line + 2U * sizeof(uint64_t);
    const uintptr_t neighbor = line + 5U * sizeof(uint64_t);
    const uintptr_t before_guard = line - sizeof(uint64_t);
    const uintptr_t after_guard = line + 64U;
    const uintptr_t marker = line + 96U;
    const uint64_t seed = 0x696d63626f6d0001ULL;
    const uint64_t dirty0 = 0x696d63626f6d1001ULL;
    const uint64_t dirty1 = 0x696d63626f6d2002ULL;
    const uint64_t post_value = 0x696d63626f6d3003ULL;
    const uint64_t before_seed = 0x696d63626f6d6701ULL;
    const uint64_t after_seed = 0x696d63626f6d6702ULL;
    const uint64_t marker_value = 0x696d63626f6d6d6bULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;
    uint64_t clean_final_read = 0ULL;
    uint64_t flush_final_read = 0ULL;
    bool initial_line_ok = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("CBO clean/flush visibility target is one aligned cacheable line with independent guards",
        (line & 63ULL) == 0ULL &&
        target < line + 64U &&
        neighbor < line + 64U,
        "line=%lx target=%lx neighbor=%lx", line, target, neighbor);

    CSRW(CSR_MENVCFG, (saved_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    CSRS(CSR_MENVCFG, MENVCFG_CBZFE);
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(line + i * sizeof(uint64_t), seed + i);
    }
    write64(before_guard, before_seed);
    write64(after_guard, after_seed);
    write64(marker, 0ULL);
    fence_iorw_iorw();
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        if (read64(line + i * sizeof(uint64_t)) != seed + i) {
            initial_line_ok = false;
        }
    }

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = target;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(target, dirty0);
    write64(neighbor, dirty1);
    TEST_SETUP_EXCEPT();
    cbo_clean(line);
    fence_iorw_iorw();
    const bool clean_retired = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds0 = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    clean_final_read = read64(target);

    write64(target, post_value);
    TEST_SETUP_EXCEPT();
    cbo_flush(line);
    fence_iorw_iorw();
    const bool flush_retired = !excpt.triggered;
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds1 = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);
    flush_final_read = read64(target);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool handler_reads_are_not_only_oracle =
        rounds0 < AI_IMC_SPIN_LIMIT &&
        rounds1 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == dirty0 &&
        ai_imc_handler_probe_value[1] == post_value &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        clean_final_read == dirty0 &&
        flush_final_read == post_value &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool final_line_visibility_ok =
        initial_line_ok &&
        clean_retired &&
        flush_retired &&
        read64(target) == post_value &&
        read64(neighbor) == dirty1 &&
        read64(line) == seed &&
        read64(line + 7U * sizeof(uint64_t)) == seed + 7ULL &&
        read64(before_guard) == before_seed &&
        read64(after_guard) == after_seed &&
        read64(marker) == marker_value;
    const bool clean_flush_sources_disjoint =
        ai_imc_handler_probe_value[0] != ai_imc_handler_log_page[0] &&
        ai_imc_handler_probe_value[1] != ai_imc_handler_log_page[1] &&
        clean_final_read == ai_imc_handler_probe_value[0] &&
        flush_final_read == ai_imc_handler_probe_value[1] &&
        read64(marker) != ai_imc_handler_probe_value[1];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("CBO clean and flush each retire before a STIP handler reads the target, and final mainline reads independently confirm the values",
        handler_reads_are_not_only_oracle,
        "rounds=%lx,%lx trap_count=%lx causes=%lx,%lx probes=%lx,%lx finals=%lx,%lx final_mip=%lx",
        rounds0, rounds1, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_cause[1], ai_imc_handler_probe_value[0],
        ai_imc_handler_probe_value[1], clean_final_read, flush_final_read,
        CSRR(CSR_MIP));
    TEST_ASSERT("dirty target, neighbor line word, guards and marker remain same-source across CBO clean/flush and interrupt handler reads",
        final_line_visibility_ok,
        "init=%d clean=%d flush=%d target=%lx neighbor=%lx line0=%lx line7=%lx guards=%lx,%lx marker=%lx",
        initial_line_ok, clean_retired, flush_retired, read64(target),
        read64(neighbor), read64(line),
        read64(line + 7U * sizeof(uint64_t)), read64(before_guard),
        read64(after_guard), read64(marker));
    TEST_ASSERT("handler read values are not accepted as CBO completion side effects or marker owners",
        clean_flush_sources_disjoint,
        "probe0=%lx probe1=%lx log0=%lx log1=%lx clean_final=%lx flush_final=%lx marker=%lx",
        ai_imc_handler_probe_value[0], ai_imc_handler_probe_value[1],
        ai_imc_handler_log_page[0], ai_imc_handler_log_page[1],
        clean_final_read, flush_final_read, read64(marker));

    TEST_END(ai_interrupt_memory_m_stip_cbom_clean_flush_handler_read_visibility_corner);
}

bool ai_interrupt_memory_m_stip_wfi_store_fence_wakeup_marker_corner()
{
    TEST_START();

    const uintptr_t pre_store = phys_page_base(SCRATCHPAD) + 0x1f00U;
    const uintptr_t post_store = phys_page_base(SCRATCHPAD) + 0x1f08U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0x1f10U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x1f18U;
    const uint64_t pre_seed = 0x696d637766697331ULL;
    const uint64_t post_seed = 0x696d637766697332ULL;
    const uint64_t pre_payload = 0x696d637766697341ULL;
    const uint64_t post_payload = 0x696d637766697342ULL;
    const uint64_t marker_value = 0x696d63776669734dULL;
    const uint64_t guard_seed = 0x696d637766697347ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t pre_after_wfi = 0ULL;
    uint64_t post_after_wfi = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(pre_store, pre_seed);
    write64(post_store, post_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(pre_store, pre_payload);
    fence_iorw_iorw();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    wfi();
    pre_after_wfi = read64(pre_store);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    write64(post_store, post_payload);
    fence_iorw_iorw();
    post_after_wfi = read64(post_store);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool wfi_interrupt_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool pre_wfi_store_ordered_before_marker =
        pre_after_wfi == pre_payload &&
        read64(pre_store) == pre_payload &&
        read64(marker) == marker_value;
    const bool post_wakeup_memory_uses_main_episode =
        post_after_wfi == post_payload &&
        read64(post_store) == post_payload &&
        read64(guard) == guard_seed &&
        marker_value != ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("pending STIP wakes the WFI window once and the handler clears the source before post-wakeup memory",
        wfi_interrupt_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("the store and fence before WFI are visible before the post-wakeup marker is written",
        pre_wfi_store_ordered_before_marker,
        "pre_after_wfi=%lx pre=%lx marker=%lx expected_pre=%lx",
        pre_after_wfi, read64(pre_store), read64(marker), pre_payload);
    TEST_ASSERT("post-wakeup store, marker and guard remain mainline memory events and do not reuse handler log data",
        post_wakeup_memory_uses_main_episode,
        "post_after_wfi=%lx post=%lx marker=%lx guard=%lx handler_log=%lx",
        post_after_wfi, read64(post_store), read64(marker),
        read64(guard), ai_imc_handler_log_page[0]);

    TEST_END(ai_interrupt_memory_m_stip_wfi_store_fence_wakeup_marker_corner);
}

bool ai_interrupt_memory_m_stip_release_acquire_payload_handler_ack_corner()
{
    TEST_START();

    const uintptr_t seq_addr = phys_page_base(SWITCH2) + 0x1fc0U;
    const uintptr_t payload_addr = phys_page_base(SWITCH2) + 0x1fc8U;
    const uintptr_t checksum_addr = phys_page_base(SWITCH2) + 0x1fd0U;
    const uintptr_t ack_addr = phys_page_base(SWITCH2) + 0x1fd8U;
    const uintptr_t guard_addr = phys_page_base(SWITCH2) + 0x1fe0U;
    const uintptr_t post_marker = phys_page_base(SWITCH2) + 0x1fe8U;
    const uint64_t old_seq = 0x10ULL;
    const uint64_t new_seq = 0x11ULL;
    const uint64_t old_payload = 0x696d6372656c6f64ULL;
    const uint64_t new_payload = 0x696d6372656e6577ULL;
    const uint64_t new_checksum =
        new_payload ^ (new_seq << 8U) ^ 0x5a5a5a5a5a5a5a5aULL;
    const uint64_t guard_seed = 0x696d637265677561ULL;
    const uint64_t marker_value = 0x696d6372656d6b21ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t first_load_after_handler = 0ULL;

    goto_priv(PRIV_M);
    write64(seq_addr, old_seq);
    write64(payload_addr, old_payload);
    write64(checksum_addr, old_payload ^ old_seq);
    write64(ack_addr, 0ULL);
    write64(guard_addr, guard_seed);
    write64(post_marker, 0ULL);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_message_seq_addr = seq_addr;
    ai_imc_message_payload_addr = payload_addr;
    ai_imc_message_checksum_addr = checksum_addr;
    ai_imc_message_ack_addr = ack_addr;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_message_ack_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    write64(payload_addr, new_payload);
    write64(checksum_addr, new_checksum);
    ai_imc_amoadd_d_aqrl((uint64_t *)seq_addr, 1ULL);
    fence_iorw_iorw();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    first_load_after_handler = read64(payload_addr);
    write64(post_marker, marker_value);
    fence_iorw_iorw();

    const bool handler_payload_generation_ok =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_message_seq_log[0] == new_seq &&
        ai_imc_message_payload_log[0] == new_payload &&
        ai_imc_message_payload_log[0] != old_payload &&
        ai_imc_message_checksum_log[0] == new_checksum &&
        ai_imc_message_checksum_log[0] ==
            (ai_imc_message_payload_log[0] ^
             (ai_imc_message_seq_log[0] << 8U) ^
             0x5a5a5a5a5a5a5a5aULL) &&
        ai_imc_message_ack_log[0] == new_seq &&
        read64(ack_addr) == new_seq &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool post_wakeup_memory_generation_ok =
        first_load_after_handler == new_payload &&
        read64(seq_addr) == new_seq &&
        read64(payload_addr) == new_payload &&
        read64(checksum_addr) == new_checksum &&
        read64(post_marker) == marker_value &&
        read64(guard_addr) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_message_seq_addr = 0ULL;
    ai_imc_message_payload_addr = 0ULL;
    ai_imc_message_checksum_addr = 0ULL;
    ai_imc_message_ack_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler reads the released payload generation, checksum, and sequence exactly once before writing ack",
        handler_payload_generation_ok,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx seq=%lx payload=%lx checksum=%lx ack_log=%lx ack=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_message_seq_log[0],
        ai_imc_message_payload_log[0], ai_imc_message_checksum_log[0],
        ai_imc_message_ack_log[0], read64(ack_addr), CSRR(CSR_MIP));
    TEST_ASSERT("post-handler first load and marker observe the same message generation rather than the old payload image",
        post_wakeup_memory_generation_ok,
        "first_payload=%lx seq=%lx payload=%lx checksum=%lx marker=%lx guard=%lx",
        first_load_after_handler, read64(seq_addr), read64(payload_addr),
        read64(checksum_addr), read64(post_marker), read64(guard_addr));

    TEST_END(ai_interrupt_memory_m_stip_release_acquire_payload_handler_ack_corner);
}

bool ai_interrupt_memory_m_stip_time_read_store_drain_marker_corner()
{
    TEST_START();

    const uintptr_t data_addr = phys_page_base(SWITCH1) + 0x1fc0U;
    const uintptr_t post_addr = phys_page_base(SWITCH1) + 0x1fc8U;
    const uintptr_t time_log_addr = phys_page_base(SWITCH1) + 0x1fd0U;
    const uintptr_t marker_addr = phys_page_base(SWITCH1) + 0x1fd8U;
    const uintptr_t guard_addr = phys_page_base(SWITCH1) + 0x1fe0U;
    const uint64_t data_seed = 0x696d6374696d6501ULL;
    const uint64_t data_payload = 0x696d6374696d6511ULL;
    const uint64_t post_payload = 0x696d6374696d6522ULL;
    const uint64_t marker_value = 0x696d6374696d6533ULL;
    const uint64_t guard_seed = 0x696d6374696d65aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t time_before = 0ULL;
    uint64_t time_after = 0ULL;
    uint64_t data_after_handler = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(data_addr, data_seed);
    write64(post_addr, 0ULL);
    write64(time_log_addr, 0ULL);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_time_log_store_addr = time_log_addr;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_time_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    time_before = CSRR(CSR_TIME);
    write64(data_addr, data_payload);
    fence_iorw_iorw();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    time_after = CSRR(CSR_TIME);
    data_after_handler = read64(data_addr);
    write64(post_addr, post_payload);
    fence_iorw_iorw();
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool time_handler_logged_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_time_log_value[0] != 0ULL &&
        read64(time_log_addr) == ai_imc_time_log_value[0] &&
        time_after >= time_before &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool main_store_ordering_uses_main_fence =
        data_after_handler == data_payload &&
        read64(data_addr) == data_payload &&
        read64(post_addr) == post_payload &&
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed &&
        read64(time_log_addr) != data_payload &&
        read64(time_log_addr) != post_payload &&
        read64(time_log_addr) != marker_value;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_time_log_store_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler reads TIME once into its own log and clears the source without using it as a memory witness",
        time_handler_logged_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx time_before=%lx time_log=%lx time_after=%lx log_store=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], time_before, ai_imc_time_log_value[0],
        time_after, read64(time_log_addr), CSRR(CSR_MIP));
    TEST_ASSERT("mainline data/post/marker stores are checked by mainline fence and readback, not by the handler time read",
        main_store_ordering_uses_main_fence,
        "data_after=%lx data=%lx post=%lx marker=%lx guard=%lx time_log=%lx",
        data_after_handler, read64(data_addr), read64(post_addr),
        read64(marker_addr), read64(guard_addr), read64(time_log_addr));

    TEST_END(ai_interrupt_memory_m_stip_time_read_store_drain_marker_corner);
}

bool ai_interrupt_memory_m_msip_mmio_source_write_handler_clear_corner()
{
    TEST_START();

    const uintptr_t clint_msip = AI_IMC_CLINT_MSIP0;
    const uintptr_t source_log = phys_page_base(SWITCH2) + 0x1f40U;
    const uintptr_t post_marker = phys_page_base(SWITCH2) + 0x1f48U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x1f50U;
    const uint64_t source_log_seed = 0x696d636d73697031ULL;
    const uint64_t post_marker_value = 0x696d636d73697032ULL;
    const uint64_t guard_seed = 0x696d636d736970aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t msip_initial = 0ULL;
    uint64_t mip_after_mmio_set = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t msip_after_handler = 0ULL;

    goto_priv(PRIV_M);
    write64(source_log, source_log_seed);
    write64(post_marker, 0ULL);
    write64(guard, guard_seed);
    write32(clint_msip, 0U);
    fence_iorw_iorw();
    msip_initial = read32(clint_msip);

    ai_imc_reset_trap_log();
    ai_imc_clint_msip_addr = clint_msip;
    ai_imc_clint_handler_log_addr = source_log;
    ai_imc_prepare_msip(1ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_clint_msip_clear_handler);

    write32(clint_msip, 1U);
    fence_iorw_iorw();
    mip_after_mmio_set = CSRR(CSR_MIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    msip_after_handler = read32(clint_msip);
    write64(post_marker, post_marker_value);
    fence_iorw_iorw();

    const bool clint_mmio_source_reached_handler =
        msip_initial == 0ULL &&
        (mip_after_mmio_set & AI_IMC_MIP_MSIP) != 0ULL &&
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_MSI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_MSIP) != 0ULL &&
        ai_imc_clint_msip_before_clear[0] == 1ULL &&
        ai_imc_clint_log_value[0] == source_log_seed &&
        ai_imc_handler_log_page[0] == 0x696d63636c6e7400ULL;
    const bool handler_clear_and_main_memory_are_isolated =
        ai_imc_clint_msip_after_clear[0] == 0ULL &&
        msip_after_handler == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_MSIP) == 0ULL &&
        read64(source_log) == source_log_seed &&
        read64(post_marker) == post_marker_value &&
        read64(guard) == guard_seed &&
        post_marker_value != ai_imc_clint_log_value[0] &&
        post_marker_value != ai_imc_handler_log_page[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    write32(clint_msip, 0U);
    fence_iorw_iorw();
    CSRC(CSR_MIP, AI_IMC_MIP_MSIP);
    ai_imc_clint_msip_addr = 0ULL;
    ai_imc_clint_handler_log_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("CLINT msip MMIO source write raises one machine software interrupt and the handler observes the source before clear",
        clint_mmio_source_reached_handler,
        "initial=%lx mip_set=%lx rounds=%lx trap_count=%lx cause=%lx mip0=%lx before=%lx log=%lx hlog=%lx",
        msip_initial, mip_after_mmio_set, rounds, ai_imc_trap_count,
        ai_imc_trap_cause[0], ai_imc_trap_mip[0],
        ai_imc_clint_msip_before_clear[0], ai_imc_clint_log_value[0],
        ai_imc_handler_log_page[0]);
    TEST_ASSERT("handler write-zero clear consumes the CLINT msip source once and does not pollute mainline memory markers",
        handler_clear_and_main_memory_are_isolated,
        "after_clear=%lx readback=%lx mip=%lx source_log=%lx marker=%lx guard=%lx",
        ai_imc_clint_msip_after_clear[0], msip_after_handler,
        CSRR(CSR_MIP), read64(source_log), read64(post_marker),
        read64(guard));

    TEST_END(ai_interrupt_memory_m_msip_mmio_source_write_handler_clear_corner);
}

bool ai_interrupt_memory_m_msip_mmio_lane_select_handler_clear_corner()
{
    TEST_START();

    const uintptr_t clint_msip = AI_IMC_CLINT_MSIP0;
    const uintptr_t source_log = phys_page_base(SWITCH2) + 0x1f58U;
    const uintptr_t post_marker = phys_page_base(SWITCH2) + 0x1f60U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x1f68U;
    const uint64_t source_log_seed = 0x696d636d736c6e31ULL;
    const uint64_t post_marker_value = 0x696d636d736c6e32ULL;
    const uint64_t guard_seed = 0x696d636d736c6eaaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t byte_read = 0ULL;
    uint64_t byte_word = 0ULL;
    uint64_t half_read = 0ULL;
    uint64_t half_word = 0ULL;
    uint64_t word_read = 0ULL;
    uint64_t word_byte0 = 0ULL;
    uint64_t word_byte1 = 0ULL;
    uint64_t mip_after_byte_set = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t msip_after_handler = 0ULL;

    goto_priv(PRIV_M);
    write64(source_log, source_log_seed);
    write64(post_marker, 0ULL);
    write64(guard, guard_seed);
    ai_imc_reset_trap_log();
    ai_imc_prepare_msip(0ULL);
    write32(clint_msip, 0U);
    fence_iorw_iorw();

    write8(clint_msip + 1U, 0x5aU);
    fence_iorw_iorw();
    byte_read = read8(clint_msip + 1U);
    byte_word = read32(clint_msip);
    write32(clint_msip, 0U);
    fence_iorw_iorw();

    write16(clint_msip + 2U, 0x00c3U);
    fence_iorw_iorw();
    half_read = read16(clint_msip + 2U);
    half_word = read32(clint_msip);
    write32(clint_msip, 0U);
    fence_iorw_iorw();

    write32(clint_msip, 0x00000001U);
    fence_iorw_iorw();
    word_read = read32(clint_msip);
    word_byte0 = read8(clint_msip);
    word_byte1 = read8(clint_msip + 1U);
    write32(clint_msip, 0U);
    fence_iorw_iorw();

    const bool clint_regfield_byte_half_word_lanes =
        ai_imc_trap_count == 0ULL &&
        byte_read == 0x5aULL &&
        byte_word == 0x00005a00ULL &&
        half_read == 0x00c3ULL &&
        half_word == 0x00c30000ULL &&
        word_read == 0x00000001ULL &&
        word_byte0 == 0x01ULL &&
        word_byte1 == 0x00ULL &&
        read32(clint_msip) == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_MSIP) == 0ULL;

    ai_imc_reset_trap_log();
    ai_imc_clint_msip_addr = clint_msip;
    ai_imc_clint_handler_log_addr = source_log;
    ai_imc_prepare_msip(1ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_clint_msip_clear_handler);

    write8(clint_msip, 1U);
    fence_iorw_iorw();
    mip_after_byte_set = CSRR(CSR_MIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    msip_after_handler = read32(clint_msip);
    write64(post_marker, post_marker_value);
    fence_iorw_iorw();

    const bool byte_source_interrupt_and_clear =
        (mip_after_byte_set & AI_IMC_MIP_MSIP) != 0ULL &&
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_MSI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_MSIP) != 0ULL &&
        ai_imc_clint_msip_before_clear[0] == 1ULL &&
        ai_imc_clint_msip_after_clear[0] == 0ULL &&
        msip_after_handler == 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_MSIP) == 0ULL &&
        ai_imc_clint_log_value[0] == source_log_seed &&
        ai_imc_handler_log_page[0] == 0x696d63636c6e7400ULL &&
        read64(source_log) == source_log_seed &&
        read64(post_marker) == post_marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    write32(clint_msip, 0U);
    fence_iorw_iorw();
    CSRC(CSR_MIP, AI_IMC_MIP_MSIP);
    ai_imc_clint_msip_addr = 0ULL;
    ai_imc_clint_handler_log_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("CLINT msip RegField.bytes responder preserves byte/half/word lane select while interrupts are disabled",
        clint_regfield_byte_half_word_lanes,
        "byte=%lx word=%lx half=%lx hword=%lx word_read=%lx b0=%lx b1=%lx msip=%x mip=%lx traps=%lx",
        byte_read, byte_word, half_read, half_word, word_read,
        word_byte0, word_byte1, read32(clint_msip), CSRR(CSR_MIP),
        ai_imc_trap_count);
    TEST_ASSERT("a byte-lane write to CLINT msip produces one MSI and the handler clears the same responder source without lane leakage",
        byte_source_interrupt_and_clear,
        "mip_set=%lx rounds=%lx trap_count=%lx cause=%lx before=%lx after=%lx readback=%lx log=%lx marker=%lx guard=%lx",
        mip_after_byte_set, rounds, ai_imc_trap_count,
        ai_imc_trap_cause[0], ai_imc_clint_msip_before_clear[0],
        ai_imc_clint_msip_after_clear[0], msip_after_handler,
        ai_imc_clint_log_value[0], read64(post_marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_msip_mmio_lane_select_handler_clear_corner);
}

bool ai_interrupt_memory_m_stip_vector_handler_context_save_restore_corner()
{
    TEST_START();

    const uintptr_t main_src = phys_page_base(SWITCH1) + 0x1ee0U;
    const uintptr_t main_dump = phys_page_base(SWITCH1) + 0x1f00U;
    const uintptr_t handler_src = phys_page_base(SWITCH2) + 0x1ee0U;
    const uintptr_t handler_dst = phys_page_base(SWITCH2) + 0x1f00U;
    const uintptr_t handler_save = phys_page_base(SCRATCHPAD) + 0x1ee0U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x1f20U;
    const uint32_t main_words[AI_IMC_VEC_VL4] = {
        0x76633010U, 0x76633021U, 0x76633032U, 0x76633043U
    };
    const uint16_t handler_words[AI_IMC_VEC_VL4] = {
        0x7101U, 0x7202U, 0x7303U, 0x7404U
    };
    const uint64_t guard_seed = 0x696d637676637461ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t main_load_vl = 0ULL;
    uint64_t main_load_vtype = 0ULL;
    uint64_t main_load_vstart = 0xffffffffffffffffULL;
    uint64_t dump_vl = 0ULL;
    uint64_t dump_vtype = 0ULL;
    uint64_t dump_vstart = 0xffffffffffffffffULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(main_src + i * sizeof(uint32_t), main_words[i]);
        write32(main_dump + i * sizeof(uint32_t), 0xeeee0000U + (uint32_t)i);
        write16(handler_src + i * sizeof(uint16_t), handler_words[i]);
        write16(handler_dst + i * sizeof(uint16_t), 0xddddU);
        write32(handler_save + i * sizeof(uint32_t),
            0xcccc0000U + (uint32_t)i);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_vec_prepare_e32m1_vl4();
    ai_imc_vec_load_current_vl_u32((const uint32_t *)main_src,
        &main_load_vl, &main_load_vtype, &main_load_vstart);

    ai_imc_reset_trap_log();
    ai_imc_handler_vec_src_addr = handler_src;
    ai_imc_handler_vec_dst_addr = handler_dst;
    ai_imc_handler_vec_save_addr = handler_save;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_vector_context_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    ai_imc_vec_store_current_vl_u32((uint32_t *)main_dump,
        &dump_vl, &dump_vtype, &dump_vstart);
    fence_iorw_iorw();

    const bool interrupt_vector_handler_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d637676637400ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool vector_state_restored =
        main_load_vl == AI_IMC_VEC_VL4 &&
        dump_vl == main_load_vl &&
        dump_vtype == main_load_vtype &&
        main_load_vstart == 0ULL &&
        dump_vstart == 0ULL &&
        ai_imc_handler_vec_saved_vl[0] == main_load_vl &&
        ai_imc_handler_vec_saved_vtype[0] == main_load_vtype &&
        ai_imc_handler_vec_saved_vstart[0] == 0ULL &&
        ai_imc_handler_vec_handler_vl[0] >= AI_IMC_VEC_VL4 &&
        ai_imc_handler_vec_handler_vtype[0] != main_load_vtype;
    const bool main_vector_register_survived =
        ai_imc_words_match(main_dump, main_words, AI_IMC_VEC_VL4) &&
        ai_imc_words_match(handler_save, main_words, AI_IMC_VEC_VL4);
    const bool handler_vector_memory_isolated =
        read16(handler_dst) == handler_words[0] &&
        read16(handler_dst + 2U) == handler_words[1] &&
        read16(handler_dst + 4U) == handler_words[2] &&
        read16(handler_dst + 6U) == handler_words[3] &&
        read64(guard) == guard_seed &&
        read32(main_dump) != (uint32_t)read16(handler_dst);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_vec_src_addr = 0ULL;
    ai_imc_handler_vec_dst_addr = 0ULL;
    ai_imc_handler_vec_save_addr = 0ULL;
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler using vector memory runs exactly once and clears the pending source",
        interrupt_vector_handler_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("handler saves and restores mainline vector vl/vtype/vstart after using a different vector element width",
        vector_state_restored,
        "main_vl=%lx dump_vl=%lx main_vtype=%lx dump_vtype=%lx saved_vl=%lx saved_vtype=%lx saved_vstart=%lx handler_vl=%lx handler_vtype=%lx",
        main_load_vl, dump_vl, main_load_vtype, dump_vtype,
        ai_imc_handler_vec_saved_vl[0],
        ai_imc_handler_vec_saved_vtype[0],
        ai_imc_handler_vec_saved_vstart[0],
        ai_imc_handler_vec_handler_vl[0],
        ai_imc_handler_vec_handler_vtype[0]);
    TEST_ASSERT("mainline v6 register payload survives the handler vector memory episode and the handler save area captures that payload",
        main_vector_register_survived,
        "dump=%x,%x,%x,%x save=%x,%x,%x,%x",
        read32(main_dump), read32(main_dump + 4U),
        read32(main_dump + 8U), read32(main_dump + 12U),
        read32(handler_save), read32(handler_save + 4U),
        read32(handler_save + 8U), read32(handler_save + 12U));
    TEST_ASSERT("handler vector load/store uses its own DRAM window and does not overwrite mainline vector memory or guard",
        handler_vector_memory_isolated,
        "handler_dst=%x,%x,%x,%x main0=%x guard=%lx",
        read16(handler_dst), read16(handler_dst + 2U),
        read16(handler_dst + 4U), read16(handler_dst + 6U),
        read32(main_dump), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vector_handler_context_save_restore_corner);
}

bool ai_interrupt_memory_m_stip_vlm_vsm_handler_mask_restore_corner()
{
    TEST_START();

    const uintptr_t main_mask = phys_page_base(SWITCH1) + 0x1f40U;
    const uintptr_t main_raw = phys_page_base(SWITCH1) + 0x1f50U;
    const uintptr_t main_src = phys_page_base(SWITCH1) + 0x1f60U;
    const uintptr_t main_dst = phys_page_base(SWITCH1) + 0x1f80U;
    const uintptr_t handler_save = phys_page_base(SWITCH2) + 0x1f40U;
    const uintptr_t handler_mask = phys_page_base(SWITCH2) + 0x1f50U;
    const uintptr_t handler_raw = phys_page_base(SWITCH2) + 0x1f60U;
    const uintptr_t handler_src = phys_page_base(SWITCH2) + 0x1f80U;
    const uintptr_t handler_dst = phys_page_base(SWITCH2) + 0x1fa0U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x1f40U;
    const uint8_t main_mask_bytes[AI_IMC_MASK_BYTES] = {
        0x35U, 0x01U
    };
    const uint8_t handler_mask_bytes[AI_IMC_MASK_BYTES] = {
        0x4aU, 0x02U
    };
    uint8_t main_src_bytes[AI_IMC_MASK_E8_BYTES];
    uint8_t main_seed_bytes[AI_IMC_MASK_E8_BYTES];
    uint8_t main_expected[AI_IMC_MASK_E8_BYTES];
    uint16_t handler_src_words[AI_IMC_MASK_E16_ELEMS];
    uint16_t handler_seed_words[AI_IMC_MASK_E16_ELEMS];
    uint16_t handler_expected[AI_IMC_MASK_E16_ELEMS];
    const uint64_t guard_seed = 0x696d63766d736b67ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t main_load_vl = 0ULL;
    uint64_t main_load_vtype = 0ULL;
    uint64_t main_load_vstart = 0xffffffffffffffffULL;
    uint64_t dump_vl = 0ULL;
    uint64_t dump_vtype = 0ULL;
    uint64_t dump_vstart = 0xffffffffffffffffULL;
    uint64_t rounds = 0ULL;

    for (uint64_t i = 0ULL; i < AI_IMC_MASK_E8_BYTES; ++i) {
        main_src_bytes[i] = (uint8_t)(0x40U + i * 7U);
        main_seed_bytes[i] = (uint8_t)(0xa0U + i);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_MASK_E16_ELEMS; ++i) {
        handler_src_words[i] = (uint16_t)(0x7100U + i * 0x11U);
        handler_seed_words[i] = (uint16_t)(0xd000U + i);
    }
    ai_imc_mask_store_expected_u8(main_expected, main_seed_bytes,
        main_src_bytes, main_mask_bytes, AI_IMC_MASK_E8_BYTES);
    ai_imc_mask_store_expected_u16(handler_expected, handler_seed_words,
        handler_src_words, handler_mask_bytes, AI_IMC_MASK_E16_ELEMS);

    goto_priv(PRIV_M);
    ai_imc_write_bytes(main_mask, main_mask_bytes, sizeof(main_mask_bytes));
    ai_imc_write_bytes(handler_mask, handler_mask_bytes,
        sizeof(handler_mask_bytes));
    for (uint64_t i = 0ULL; i < AI_IMC_MASK_E8_BYTES; ++i) {
        write8(main_raw + i, 0xeeU);
        write8(main_src + i, main_src_bytes[i]);
        write8(main_dst + i, main_seed_bytes[i]);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_MASK_BYTES; ++i) {
        write8(handler_save + i, 0xccU);
        write8(handler_raw + i, 0xddU);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_MASK_E16_ELEMS; ++i) {
        write16(handler_src + i * sizeof(uint16_t), handler_src_words[i]);
        write16(handler_dst + i * sizeof(uint16_t),
            handler_seed_words[i]);
    }
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_mask_load_v0_e8_vl9((const uint8_t *)main_mask,
        &main_load_vl, &main_load_vtype, &main_load_vstart);

    ai_imc_reset_trap_log();
    ai_imc_handler_mask_save_addr = handler_save;
    ai_imc_handler_mask_src_addr = handler_mask;
    ai_imc_handler_mask_dump_addr = handler_raw;
    ai_imc_handler_mask_payload_addr = handler_src;
    ai_imc_handler_mask_target_addr = handler_dst;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_mask_context_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    TEST_SETUP_EXCEPT();
    ai_imc_mask_dump_and_store_e8_vl9((const uint8_t *)main_src,
        (uint8_t *)main_raw, (uint8_t *)main_dst, &dump_vl,
        &dump_vtype, &dump_vstart);
    const bool main_mask_consumer_no_fault = !excpt.triggered;
    fence_iorw_iorw();

    const bool interrupt_mask_handler_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63766d736b00ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool main_v0_restored =
        main_mask_consumer_no_fault &&
        main_load_vl == AI_IMC_MASK_VL9 &&
        dump_vl == main_load_vl &&
        dump_vtype == main_load_vtype &&
        main_load_vstart == 0ULL &&
        dump_vstart == 0ULL &&
        ai_imc_handler_vec_saved_vl[0] == main_load_vl &&
        ai_imc_handler_vec_saved_vtype[0] == main_load_vtype &&
        ai_imc_handler_vec_saved_vstart[0] == 0ULL &&
        ai_imc_handler_vec_handler_vl[0] == AI_IMC_MASK_VL9 &&
        ai_imc_handler_vec_handler_vtype[0] != main_load_vtype;
    const bool main_mask_image_ok =
        ai_imc_bytes_match(main_raw, main_mask_bytes,
            AI_IMC_MASK_BYTES) &&
        ai_imc_bytes_match(main_dst, main_expected,
            AI_IMC_MASK_E8_BYTES) &&
        read8(main_raw + AI_IMC_MASK_BYTES) == 0xeeU;
    const bool handler_mask_image_ok =
        ai_imc_bytes_match(handler_save, main_mask_bytes,
            AI_IMC_MASK_BYTES) &&
        ai_imc_bytes_match(handler_raw, handler_mask_bytes,
            AI_IMC_MASK_BYTES) &&
        ai_imc_halfwords_match(handler_dst, handler_expected,
            AI_IMC_MASK_E16_ELEMS) &&
        read64(guard) == guard_seed &&
        read8(handler_save) != handler_mask_bytes[0] &&
        read8(main_raw) != handler_mask_bytes[0];

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_mask_save_addr = 0ULL;
    ai_imc_handler_mask_src_addr = 0ULL;
    ai_imc_handler_mask_dump_addr = 0ULL;
    ai_imc_handler_mask_payload_addr = 0ULL;
    ai_imc_handler_mask_target_addr = 0ULL;
    CSRW(CSR_VSTART, 0ULL);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler saves mainline v0, uses a different mask image, restores v0, and clears the interrupt source exactly once",
        interrupt_mask_handler_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("mainline vlm/vsm context is restored after handler mask memory changes vl/vtype and uses v0 as a handler scratch mask",
        main_v0_restored,
        "main_vl=%lx dump_vl=%lx main_vtype=%lx dump_vtype=%lx saved_vl=%lx saved_vtype=%lx handler_vl=%lx handler_vtype=%lx vstart=%lx/%lx saved_vstart=%lx",
        main_load_vl, dump_vl, main_load_vtype, dump_vtype,
        ai_imc_handler_vec_saved_vl[0],
        ai_imc_handler_vec_saved_vtype[0],
        ai_imc_handler_vec_handler_vl[0],
        ai_imc_handler_vec_handler_vtype[0], main_load_vstart,
        dump_vstart, ai_imc_handler_vec_saved_vstart[0]);
    TEST_ASSERT("restored mainline v0 byte image drives only the expected masked e8 lanes and leaves mask tail bytes undisturbed",
        main_mask_image_ok,
        "raw=%x,%x,%x dst=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
        read8(main_raw), read8(main_raw + 1U), read8(main_raw + 2U),
        read8(main_dst), read8(main_dst + 1U), read8(main_dst + 2U),
        read8(main_dst + 3U), read8(main_dst + 4U),
        read8(main_dst + 5U), read8(main_dst + 6U),
        read8(main_dst + 7U), read8(main_dst + 8U),
        read8(main_dst + 9U), read8(main_dst + 10U),
        read8(main_dst + 11U));
    TEST_ASSERT("handler vsm/vlm scratch memory captures the saved main mask separately from the handler mask and cannot ghost into mainline v0 consumers",
        handler_mask_image_ok,
        "save=%x,%x raw=%x,%x hdst=%x,%x,%x,%x,%x,%x,%x,%x,%x,%x guard=%lx",
        read8(handler_save), read8(handler_save + 1U), read8(handler_raw),
        read8(handler_raw + 1U), read16(handler_dst),
        read16(handler_dst + 2U), read16(handler_dst + 4U),
        read16(handler_dst + 6U), read16(handler_dst + 8U),
        read16(handler_dst + 10U), read16(handler_dst + 12U),
        read16(handler_dst + 14U), read16(handler_dst + 16U),
        read16(handler_dst + 18U), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_vlm_vsm_handler_mask_restore_corner);
}

bool ai_interrupt_memory_hs_stip_superpage_demotion_promotion_rewalk_corner()
{
    TEST_START();

    const uintptr_t super_vaddr_base = 0x00400000ULL;
    const uintptr_t access_offset = 0x3000ULL;
    const uintptr_t access_vaddr = super_vaddr_base + access_offset;
    const uintptr_t super_paddr_base = TEST_PPAGE_BASE;
    const uintptr_t child_paddr_base = phys_page_base(SCRATCHPAD);
    const uintptr_t promote_paddr_base = TEST_PPAGE_BASE;
    const uintptr_t super_paddr = super_paddr_base + access_offset;
    const uintptr_t child_paddr =
        child_paddr_base + (access_offset & (PAGE_SIZE - 1ULL));
    const uintptr_t promote_paddr = promote_paddr_base + access_offset;
    const uintptr_t guard_paddr = phys_page_base(SWITCH2) + 0x1f00U;
    const uint64_t super_seed = 0x696d6373706f6c64ULL;
    const uint64_t child_seed = 0x696d63737063686cULL;
    const uint64_t child_store = 0x696d63737064656dULL;
    const uint64_t promote_store = 0x696d63737070726fULL;
    const uint64_t guard_seed = 0x696d637370677561ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t sv39_satp =
        AI_IMC_SATP_MODE_SV39 |
        (((uintptr_t)ai_imc_super_sv39_root) >> PAGE_SHIFT);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t super_first = 0ULL;
    uint64_t child_first = 0ULL;
    uint64_t child_after_store = 0ULL;
    uint64_t promote_first = 0ULL;
    uint64_t promote_after_store = 0ULL;
    uint64_t demote_rounds = 0ULL;
    uint64_t promote_rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    ai_imc_zero_pt(ai_imc_super_sv39_root);
    ai_imc_zero_pt(ai_imc_super_sv39_l1);
    ai_imc_zero_pt(ai_imc_super_sv39_l0);
    write64(super_paddr, super_seed);
    write64(child_paddr, child_seed);
    write64(guard_paddr, guard_seed);
    fence_iorw_iorw();

    ai_imc_super_sv39_root[AI_IMC_SV39_L2_INDEX(super_vaddr_base)] =
        PTE_V | (((uintptr_t)ai_imc_super_sv39_l1) >> 2U);
    ai_imc_super_sv39_root[AI_IMC_SV39_L2_INDEX(MEM_BASE)] =
        (MEM_BASE >> 2U) | leaf_flags;
    ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)] =
        (super_paddr_base >> 2U) | leaf_flags;
    ai_imc_super_sv39_l0[AI_IMC_SV39_L0_INDEX(access_vaddr)] =
        ((child_paddr_base & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
    const uint64_t super_leaf =
        ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)];
    const uint64_t child_leaf =
        ai_imc_super_sv39_l0[AI_IMC_SV39_L0_INDEX(access_vaddr)];
    CSRW(CSR_SATP, sv39_satp);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    TEST_SETUP_EXCEPT();
    super_first = ai_imc_ld_probe(access_vaddr, 0ULL);
    const bool initial_superpage_visible =
        !excpt.triggered &&
        super_first == super_seed;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)] =
        PTE_V | (((uintptr_t)ai_imc_super_sv39_l0) >> 2U);
    fence_iorw_iorw();
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    demote_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    child_first = ai_imc_ld_probe(access_vaddr, 0ULL);
    sd(access_vaddr, child_store);
    child_after_store = ai_imc_ld_probe(access_vaddr, 0ULL);
    const bool demoted_child_visible =
        !excpt.triggered &&
        child_first == child_seed &&
        child_after_store == child_store;

    goto_priv(PRIV_M);
    ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)] =
        super_leaf;
    write64(promote_paddr, super_seed);
    fence_iorw_iorw();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    promote_rounds = ai_imc_spin_until_log_count(2ULL,
        AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    sfence_vma();

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    promote_first = ai_imc_ld_probe(access_vaddr, 0ULL);
    sd(access_vaddr, promote_store);
    promote_after_store = ai_imc_ld_probe(access_vaddr, 0ULL);
    const bool promoted_superpage_visible =
        !excpt.triggered &&
        promote_first == super_seed &&
        promote_after_store == promote_store;

    goto_priv(PRIV_M);
    const bool demotion_interrupt_once =
        demote_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count >= 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL;
    const bool promotion_interrupt_once =
        promote_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 2ULL &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool backing_images_isolated =
        ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)] ==
            super_leaf &&
        ai_imc_super_sv39_l0[AI_IMC_SV39_L0_INDEX(access_vaddr)] ==
            child_leaf &&
        read64(super_paddr) == promote_store &&
        read64(child_paddr) == child_store &&
        read64(guard_paddr) == guard_seed;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("initial Sv39 L1 superpage leaf maps the target VA to the superpage backing image",
        initial_superpage_visible,
        "super_first=%lx seed=%lx leaf=%lx satp=%lx",
        super_first, super_seed, super_leaf, sv39_satp);
    TEST_ASSERT("STIP between superpage demotion PTE update and SFENCE.VMA is delivered once before the child-page access",
        demotion_interrupt_once,
        "rounds=%lx trap_count=%lx cause0=%lx mip0=%lx",
        demote_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0]);
    TEST_ASSERT("after demotion and SFENCE.VMA, the same VA uses the 4KB child PTE and stores only to the child backing image",
        demoted_child_visible,
        "child_first=%lx child_after=%lx expected_seed=%lx expected_store=%lx",
        child_first, child_after_store, child_seed, child_store);
    TEST_ASSERT("STIP between promotion PTE update and SFENCE.VMA is delivered once before the restored superpage access",
        promotion_interrupt_once,
        "rounds=%lx trap_count=%lx cause1=%lx mip1=%lx final_mip=%lx",
        promote_rounds, ai_imc_trap_count, ai_imc_trap_cause[1],
        ai_imc_trap_mip[1], CSRR(CSR_MIP));
    TEST_ASSERT("after promotion and SFENCE.VMA, the same VA returns to the superpage backing without mixing child and superpage side effects",
        promoted_superpage_visible && backing_images_isolated,
        "promote_first=%lx promote_after=%lx super_pa=%lx child_pa=%lx guard=%lx l1=%lx l0=%lx",
        promote_first, promote_after_store, read64(super_paddr),
        read64(child_paddr), read64(guard_paddr),
        ai_imc_super_sv39_l1[AI_IMC_SV39_L1_INDEX(super_vaddr_base)],
        ai_imc_super_sv39_l0[AI_IMC_SV39_L0_INDEX(access_vaddr)]);

    TEST_END(ai_interrupt_memory_hs_stip_superpage_demotion_promotion_rewalk_corner);
}

bool ai_interrupt_memory_m_stip_fault_then_post_handler_dense_lsq_burst_corner()
{
    TEST_START();

    const enum test_page fault_page = VSRWX_GRWX;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0x1c80U;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0x1c80U;
    const uintptr_t fault_guard = fault_paddr + sizeof(uint64_t);
    const uintptr_t scalar_base = phys_page_base(SWITCH1) + 0x1c80U;
    const uintptr_t vector_src = phys_page_base(SWITCH1) + 0x1cc0U;
    const uintptr_t vector_dst = phys_page_base(SWITCH1) + 0x1ce0U;
    const uintptr_t amo_addr = phys_page_base(SWITCH1) + 0x1d00U;
    const uintptr_t marker_addr = phys_page_base(SWITCH1) + 0x1d08U;
    const uintptr_t guard_addr = phys_page_base(SWITCH1) + 0x1d10U;
    const uint64_t fault_seed = 0x696d636c73716601ULL;
    const uint64_t fault_store = 0x696d636c737166ddULL;
    const uint64_t fault_guard_seed = 0x696d636c737166aaULL;
    const uint64_t scalar_seed0 = 0x696d636c73713000ULL;
    const uint64_t scalar_seed1 = 0x696d636c73713100ULL;
    const uint64_t scalar_store0 = 0x696d636c73713055ULL;
    const uint64_t scalar_store1 = 0x696d636c73713166ULL;
    const uint32_t vector_words[AI_IMC_VEC_VL4] = {
        0x6c737130U, 0x6c737131U, 0x6c737132U, 0x6c737133U
    };
    const uint64_t amo_seed = 0x20ULL;
    const uint64_t amo_addend = 0x7ULL;
    const uint64_t marker_value = 0x696d636c73716d21ULL;
    const uint64_t guard_seed = 0x696d636c73716721ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    const uintptr_t pmp_start = fault_paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t scalar_load0 = 0ULL;
    uint64_t scalar_load1 = 0ULL;
    uint64_t vector_vstart = 0xffffffffffffffffULL;
    uint64_t amo_old = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(fault_paddr, fault_seed);
    write64(fault_guard, fault_guard_seed);
    write64(scalar_base, scalar_seed0);
    write64(scalar_base + 8U, scalar_seed1);
    write64(scalar_base + 16U, 0ULL);
    write64(scalar_base + 24U, 0ULL);
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(vector_src + i * sizeof(uint32_t), vector_words[i]);
        write32(vector_dst + i * sizeof(uint32_t),
            0xeeee0000U + (uint32_t)i);
    }
    write64(amo_addr, amo_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_store);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    pending_after_fault = CSRR(CSR_MIP);

    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MTVEC, saved_mtvec);

    scalar_load0 = read64(scalar_base);
    write64(scalar_base + 16U, scalar_store0);
    scalar_load1 = read64(scalar_base + 8U);
    write64(scalar_base + 24U, scalar_store1);
    ai_imc_enable_vector_state();
    ai_imc_vec_prepare_e32m1_vl4();
    ai_imc_vec_copy_current_vl_u32((const uint32_t *)vector_src,
        (uint32_t *)vector_dst, &vector_vstart);
    amo_old = ai_imc_amoadd_d_aqrl((uint64_t *)amo_addr, amo_addend);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool older_fault_and_pending_isolated =
        fault_cause == CAUSE_SAF &&
        fault_tval == fault_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        read64(fault_paddr) == fault_seed &&
        read64(fault_guard) == fault_guard_seed;
    const bool interrupt_after_fault_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool post_handler_scalar_owner_ok =
        scalar_load0 == scalar_seed0 &&
        scalar_load1 == scalar_seed1 &&
        read64(scalar_base + 16U) == scalar_store0 &&
        read64(scalar_base + 24U) == scalar_store1 &&
        read64(scalar_base) == scalar_seed0 &&
        read64(scalar_base + 8U) == scalar_seed1;
    const bool post_handler_vector_amo_owner_ok =
        vector_vstart == 0ULL &&
        ai_imc_words_match(vector_dst, vector_words, AI_IMC_VEC_VL4) &&
        amo_old == amo_seed &&
        read64(amo_addr) == amo_seed + amo_addend &&
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("older PMP-denied store faults once, leaves its target unchanged, and preserves pending STIP for the post-fault interrupt",
        older_fault_and_pending_isolated,
        "cause=%lx tval=%lx pending=%lx fault_mem=%lx guard=%lx",
        fault_cause, fault_tval, pending_after_fault,
        read64(fault_paddr), read64(fault_guard));
    TEST_ASSERT("STIP after the fault is delivered exactly once and clears before the dense post-handler memory burst",
        interrupt_after_fault_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("post-handler scalar loads and stores use fresh owners instead of stale fault or handler slots",
        post_handler_scalar_owner_ok,
        "load0=%lx load1=%lx store0=%lx store1=%lx seed0=%lx seed1=%lx",
        scalar_load0, scalar_load1, read64(scalar_base + 16U),
        read64(scalar_base + 24U), read64(scalar_base),
        read64(scalar_base + 8U));
    TEST_ASSERT("post-handler vector store/load and AMO complete once with independent payload, marker, and guard ownership",
        post_handler_vector_amo_owner_ok,
        "vstart=%lx vec=%x,%x,%x,%x amo_old=%lx amo_final=%lx marker=%lx guard=%lx",
        vector_vstart, read32(vector_dst), read32(vector_dst + 4U),
        read32(vector_dst + 8U), read32(vector_dst + 12U),
        amo_old, read64(amo_addr), read64(marker_addr),
        read64(guard_addr));

    TEST_END(ai_interrupt_memory_m_stip_fault_then_post_handler_dense_lsq_burst_corner);
}

bool ai_interrupt_memory_m_stip_pmp_overlap_reconfig_retry_priority_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t paddr = phys_page_base(target_page) + 0x1f40U;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x1f40U;
    const uintptr_t page_base = paddr & PAGE_ADDR_MSK;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uint64_t seed = 0x696d63706d706f31ULL;
    const uint64_t denied_payload = 0x696d63706d706fddULL;
    const uint64_t allowed_payload = 0x696d63706d706f41ULL;
    const uint64_t guard_seed = 0x696d63706d706f47ULL;
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
        ai_imc_pmp_overlap_allow_all_cfg(AI_IMC_PMP_NAPOT_NO_W);
    const uint64_t allow_cfg =
        ai_imc_pmp_overlap_allow_all_cfg(AI_IMC_PMP_NAPOT_RWX);
    uint64_t deny_fault_cause = 0ULL;
    uint64_t deny_fault_tval = 0ULL;
    uint64_t pmpcfg_deny = 0ULL;
    uint64_t pmpcfg_allow = 0ULL;
    uint64_t pending_after_deny = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();

    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, ai_imc_pmp_napot4k_addr(page_base));
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, deny_cfg);
    pmpcfg_deny = CSRR(CSR_PMPCFG0);

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, denied_payload);
    ai_imc_clear_mprv_translation();
    deny_fault_cause = excpt.cause;
    deny_fault_tval = excpt.tval;
    pending_after_deny = CSRR(CSR_MIP);

    const bool lower_priority_allow_does_not_override_deny =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        deny_fault_cause == CAUSE_SAF &&
        deny_fault_tval == vaddr &&
        pmpcfg_deny == deny_cfg &&
        (pending_after_deny & AI_IMC_MIP_STIP) != 0ULL &&
        read64(paddr) == seed &&
        read64(guard) == guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    CSRW(CSR_PMPCFG0, allow_cfg);
    pmpcfg_allow = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allowed_payload);
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();

    const bool interrupt_between_pmp_generations_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool retry_uses_new_top_priority_entry =
        !excpt.triggered &&
        pmpcfg_allow == allow_cfg &&
        read64(paddr) == allowed_payload &&
        read64(paddr) != denied_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("overlapping PMP entries honor the highest-priority deny entry and leave the store target unchanged while STIP is pending",
        lower_priority_allow_does_not_override_deny,
        "cause=%lx tval=%lx expected=%lx cfg=%lx deny_cfg=%lx pending=%lx target=%lx guard=%lx",
        deny_fault_cause, deny_fault_tval, vaddr, pmpcfg_deny,
        deny_cfg, pending_after_deny, read64(paddr), read64(guard));
    TEST_ASSERT("the preserved STIP between PMP generations is delivered exactly once and clears its source",
        interrupt_between_pmp_generations_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], CSRR(CSR_MIP));
    TEST_ASSERT("after reconfiguring the overlapping top-priority entry to allow, retry store commits exactly once with the new permission",
        retry_uses_new_top_priority_entry,
        "cfg=%lx allow_cfg=%lx target=%lx denied=%lx guard=%lx",
        pmpcfg_allow, allow_cfg, read64(paddr), denied_payload,
        read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_pmp_overlap_reconfig_retry_priority_corner);
}

bool ai_interrupt_memory_m_stip_same_block_partial_store_storm_generation_corner()
{
    TEST_START();

    const uintptr_t line = phys_page_base(SWITCH1) + 0x1f80U;
    const uintptr_t neighbor = phys_page_base(SWITCH2) + 0x1f80U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1fc0U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1fc8U;
    const uint64_t guard_seed = 0x696d637362737447ULL;
    const uint64_t marker_value = 0x696d63736273744dULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t bad_index = 64ULL;
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;
    uint64_t rounds2 = 0ULL;
    bool line_image_ok = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("same-block partial-store storm line is 64-byte aligned and separated from neighbor pressure line",
        (line & 63ULL) == 0ULL &&
        (neighbor & 63ULL) == 0ULL &&
        line != neighbor,
        "line=%lx neighbor=%lx", line, neighbor);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        write8(line + i, (uint8_t)(0x20U + i));
        write8(neighbor + i, (uint8_t)(0x80U + i));
    }
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    sb(line + 3U, 0xa3U);
    sh(line + 10U, 0xb4c5U);
    sw(neighbor + 16U, 0x11223344U);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    rounds0 = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    sw(line + 20U, 0xd6e7f809U);
    sd(line + 32U, 0x0123456789abcdefULL);
    sh(neighbor + 24U, 0x5566U);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    rounds1 = ai_imc_spin_until_log_count(2ULL, AI_IMC_SPIN_LIMIT);

    sb(line + 3U, 0x5aU);
    sh(line + 40U, 0x6b7cU);
    sw(line + 48U, 0x98badcfeU);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    rounds2 = ai_imc_spin_until_log_count(3ULL, AI_IMC_SPIN_LIMIT);
    write64(marker, marker_value);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        uint8_t expected = (uint8_t)(0x20U + i);
        if (i == 3ULL) {
            expected = 0x5aU;
        } else if (i == 10ULL) {
            expected = 0xc5U;
        } else if (i == 11ULL) {
            expected = 0xb4U;
        } else if (i == 20ULL) {
            expected = 0x09U;
        } else if (i == 21ULL) {
            expected = 0xf8U;
        } else if (i == 22ULL) {
            expected = 0xe7U;
        } else if (i == 23ULL) {
            expected = 0xd6U;
        } else if (i >= 32ULL && i < 40ULL) {
            const uint64_t shift = (i - 32ULL) * 8ULL;
            expected = (uint8_t)(0x0123456789abcdefULL >> shift);
        } else if (i == 40ULL) {
            expected = 0x7cU;
        } else if (i == 41ULL) {
            expected = 0x6bU;
        } else if (i == 48ULL) {
            expected = 0xfeU;
        } else if (i == 49ULL) {
            expected = 0xdcU;
        } else if (i == 50ULL) {
            expected = 0xbaU;
        } else if (i == 51ULL) {
            expected = 0x98U;
        }
        if (read8(line + i) != expected) {
            bad_index = i;
            line_image_ok = false;
            break;
        }
    }

    const bool storm_logged_three_times =
        rounds0 < AI_IMC_SPIN_LIMIT &&
        rounds1 < AI_IMC_SPIN_LIMIT &&
        rounds2 < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 3ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        ai_imc_trap_cause[2] == CAUSE_STI &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0x696d63686c6f6701ULL &&
        ai_imc_handler_log_page[2] == 0x696d63686c6f6702ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool neighbor_pressure_and_guards_stable =
        read8(neighbor + 16U) == 0x44U &&
        read8(neighbor + 17U) == 0x33U &&
        read8(neighbor + 18U) == 0x22U &&
        read8(neighbor + 19U) == 0x11U &&
        read8(neighbor + 24U) == 0x66U &&
        read8(neighbor + 25U) == 0x55U &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("three STIP rounds interleaved with same-block partial stores are logged once each and clear source state",
        storm_logged_three_times,
        "rounds=%lx,%lx,%lx trap_count=%lx causes=%lx,%lx,%lx logs=%lx,%lx,%lx final_mip=%lx",
        rounds0, rounds1, rounds2, ai_imc_trap_count,
        ai_imc_trap_cause[0], ai_imc_trap_cause[1],
        ai_imc_trap_cause[2], ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1], ai_imc_handler_log_page[2],
        CSRR(CSR_MIP));
    TEST_ASSERT("same-block final byte matrix uses the last store generation for overlapping lanes and keeps untouched bytes from the seed image",
        line_image_ok,
        "bad_index=%lx line0=%lx line2=%lx line4=%lx line6=%lx",
        bad_index, read64(line), read64(line + 16U),
        read64(line + 32U), read64(line + 48U));
    TEST_ASSERT("neighbor pressure stores, marker and guard stay separate from the same-block partial-store storm image",
        neighbor_pressure_and_guards_stable,
        "neighbor16=%x_%x_%x_%x neighbor24=%x_%x marker=%lx guard=%lx",
        read8(neighbor + 16U), read8(neighbor + 17U),
        read8(neighbor + 18U), read8(neighbor + 19U),
        read8(neighbor + 24U), read8(neighbor + 25U),
        read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_same_block_partial_store_storm_generation_corner);
}

bool ai_interrupt_memory_m_stip_same_line_handler_store_merge_image_corner()
{
    TEST_START();

    const uintptr_t line = phys_page_base(SWITCH2) + 0x1b80U;
    const uintptr_t neighbor = phys_page_base(SWITCH1) + 0x1b80U;
    const uintptr_t marker = line + 64U;
    const uintptr_t guard = line + 72U;
    const uint64_t post_dword = 0x0123456789abcdefULL;
    const uint64_t marker_value = 0x696d63736c686d4dULL;
    const uint64_t guard_seed = 0x696d63736c686d47ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t bad_index = 64ULL;
    bool final_line_ok = true;

    goto_priv(PRIV_M);
    TEST_ASSERT("same-line handler-store merge target is line aligned and separated from the neighbor pressure line",
        (line & 63ULL) == 0ULL &&
        (neighbor & 63ULL) == 0ULL &&
        line != neighbor,
        "line=%lx neighbor=%lx", line, neighbor);
    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        write8(line + i, (uint8_t)(0x30U + i));
        write8(neighbor + i, (uint8_t)(0x90U + i));
    }
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_store_addr = line + 40U;
    ai_imc_handler_store_value = 0x7bULL;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_partial_store_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    sb(line + 3U, 0xa3U);
    sh(line + 10U, 0xb4c5U);
    sw(line + 28U, 0xd6e7f809U);
    sw(neighbor + 16U, 0x11223344U);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    sd(line + 48U, post_dword);
    sh(neighbor + 24U, 0x5566U);
    write64(marker, marker_value);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < 64ULL; ++i) {
        uint8_t expected = (uint8_t)(0x30U + i);
        if (i == 3ULL) {
            expected = 0xa3U;
        } else if (i == 10ULL) {
            expected = 0xc5U;
        } else if (i == 11ULL) {
            expected = 0xb4U;
        } else if (i == 28ULL) {
            expected = 0x09U;
        } else if (i == 29ULL) {
            expected = 0xf8U;
        } else if (i == 30ULL) {
            expected = 0xe7U;
        } else if (i == 31ULL) {
            expected = 0xd6U;
        } else if (i == 40ULL) {
            expected = 0x7bU;
        } else if (i >= 48ULL && i < 56ULL) {
            expected = (uint8_t)(post_dword >> ((i - 48ULL) * 8ULL));
        }
        if (read8(line + i) != expected) {
            bad_index = i;
            final_line_ok = false;
            break;
        }
    }

    const bool interrupt_handler_store_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read8(line + 40U) == 0x7bU &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool neighbor_marker_and_guard_ok =
        read8(neighbor + 16U) == 0x44U &&
        read8(neighbor + 17U) == 0x33U &&
        read8(neighbor + 18U) == 0x22U &&
        read8(neighbor + 19U) == 0x11U &&
        read8(neighbor + 24U) == 0x66U &&
        read8(neighbor + 25U) == 0x55U &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_store_addr = 0ULL;
    ai_imc_handler_store_value = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("STIP handler writes exactly one byte in the same cacheline and clears the pending source",
        interrupt_handler_store_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx byte40=%x log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], read8(line + 40U),
        ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("main partial stores, handler same-line byte store and post-return store compose one deterministic final byte image",
        final_line_ok,
        "bad_index=%lx line0=%lx line2=%lx line4=%lx line6=%lx",
        bad_index, read64(line), read64(line + 16U),
        read64(line + 32U), read64(line + 48U));
    TEST_ASSERT("neighbor pressure stores, post marker and guard stay separate from the same-line merge target",
        neighbor_marker_and_guard_ok,
        "neighbor16=%x_%x_%x_%x neighbor24=%x_%x marker=%lx guard=%lx",
        read8(neighbor + 16U), read8(neighbor + 17U),
        read8(neighbor + 18U), read8(neighbor + 19U),
        read8(neighbor + 24U), read8(neighbor + 25U),
        read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_same_line_handler_store_merge_image_corner);
}

bool ai_interrupt_memory_m_stip_same_block_load_prefetch_result_matrix_corner()
{
    TEST_START();

    const uintptr_t line = phys_page_base(SWITCH1) + 0x1b80U;
    const uintptr_t handler_probe = phys_page_base(SWITCH2) + 0x1b80U;
    const uintptr_t marker = line + 64U;
    const uintptr_t guard = line + 72U;
    const uint64_t words[4] = {
        0x696d636d71773030ULL,
        0x696d636d71773131ULL,
        0x696d636d71773232ULL,
        0x696d636d71773333ULL,
    };
    const uint64_t handler_seed = 0x696d636d71776864ULL;
    const uint64_t marker_value = 0x696d636d71776d6bULL;
    const uint64_t guard_seed = 0x696d636d71776764ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t older0 = 0ULL;
    uint64_t older1 = 0ULL;
    uint64_t younger0 = 0ULL;
    uint64_t younger1 = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("same-block load/prefetch matrix uses one aligned line and an independent handler probe line",
        (line & 63ULL) == 0ULL &&
        (handler_probe & 63ULL) == 0ULL &&
        line != handler_probe,
        "line=%lx handler=%lx", line, handler_probe);
    for (uint64_t i = 0ULL; i < 4ULL; ++i) {
        write64(line + i * sizeof(uint64_t), words[i]);
    }
    write64(handler_probe, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    prefetch_r(line);
    prefetch_r(line + 16U);
    prefetch_w(marker);
    const bool prefetches_are_silent =
        !excpt.triggered &&
        read64(line) == words[0] &&
        read64(line + 16U) == words[2] &&
        read64(marker) == 0ULL &&
        read64(guard) == guard_seed;

    older0 = read64(line);
    older1 = read64(line + sizeof(uint64_t));
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    younger0 = read64(line + 2U * sizeof(uint64_t));
    younger1 = read64(line + 3U * sizeof(uint64_t));
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool same_block_result_matrix_ok =
        prefetches_are_silent &&
        older0 == words[0] &&
        older1 == words[1] &&
        younger0 == words[2] &&
        younger1 == words[3] &&
        read64(line) == words[0] &&
        read64(line + 8U) == words[1] &&
        read64(line + 16U) == words[2] &&
        read64(line + 24U) == words[3];
    const bool interrupt_handler_probe_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_probe_value[0] != words[0] &&
        ai_imc_handler_probe_value[0] != words[1] &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool marker_and_guard_ok =
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        read64(handler_probe) == handler_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("same-block prefetch hints are silent and older/younger loads keep a stable per-lane result matrix",
        same_block_result_matrix_ok,
        "older=%lx,%lx younger=%lx,%lx line=%lx,%lx,%lx,%lx",
        older0, older1, younger0, younger1,
        read64(line), read64(line + 8U),
        read64(line + 16U), read64(line + 24U));
    TEST_ASSERT("STIP handler probes its own line and cannot supply same-block mainline load results",
        interrupt_handler_probe_isolated,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx handler=%lx mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], handler_seed, CSRR(CSR_MIP));
    TEST_ASSERT("same-block load matrix leaves marker, guard and handler probe memory in separate owners",
        marker_and_guard_ok,
        "marker=%lx guard=%lx handler=%lx",
        read64(marker), read64(guard), read64(handler_probe));

    TEST_END(ai_interrupt_memory_m_stip_same_block_load_prefetch_result_matrix_corner);
}

bool ai_interrupt_memory_m_stip_spfctl_disable_reenable_stream_owner_corner()
{
    TEST_START();

    const uintptr_t stream_base = phys_page_base(SWITCH1) + 0x1000U;
    const uintptr_t disabled_base = phys_page_base(SWITCH2) + 0x1000U;
    const uintptr_t handler_probe = phys_page_base(SCRATCHPAD) + 0x1000U;
    const uintptr_t marker = phys_page_base(SCRATCHPAD) + 0x1040U;
    const uintptr_t guard = phys_page_base(SCRATCHPAD) + 0x1048U;
    const uint64_t stream_seed = 0x696d637370667331ULL;
    const uint64_t disabled_seed = 0x696d637370667332ULL;
    const uint64_t handler_seed = 0x696d637370667368ULL;
    const uint64_t marker_value = 0x696d63737066736dULL;
    const uint64_t guard_seed = 0x696d637370667367ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_spfctl = ai_imc_read_spfctl();
    uint64_t enabled_spfctl = 0ULL;
    uint64_t disabled_spfctl = 0ULL;
    uint64_t observed_enabled = 0ULL;
    uint64_t observed_disabled = 0ULL;
    uint64_t observed_reenabled = 0ULL;
    uint64_t warmup0 = 0ULL;
    uint64_t warmup1 = 0ULL;
    uint64_t disabled0 = 0ULL;
    uint64_t disabled1 = 0ULL;
    uint64_t reenabled0 = 0ULL;
    uint64_t reenabled1 = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    TEST_ASSERT("spfctl disable/reenable stream test uses 64-byte-aligned independent lines",
        (stream_base & 63ULL) == 0ULL &&
        (disabled_base & 63ULL) == 0ULL &&
        (handler_probe & 63ULL) == 0ULL &&
        stream_base != disabled_base &&
        stream_base != handler_probe &&
        disabled_base != handler_probe,
        "stream=%lx disabled=%lx handler=%lx",
        stream_base, disabled_base, handler_probe);

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        write64(stream_base + i * 64ULL, stream_seed + i);
        write64(disabled_base + i * 64ULL, disabled_seed + i);
    }
    write64(handler_probe, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    enabled_spfctl = (saved_spfctl |
        AI_IMC_SPFCTL_L2_PF_ENABLE |
        AI_IMC_SPFCTL_L1D_PF_ENABLE |
        AI_IMC_SPFCTL_TRAIN_ON_HIT |
        AI_IMC_SPFCTL_ENABLE_AGT |
        AI_IMC_SPFCTL_ENABLE_PHT);
    enabled_spfctl = (enabled_spfctl &
        ~(AI_IMC_SPFCTL_ACTIVE_THRESHOLD_MASK |
          AI_IMC_SPFCTL_ACTIVE_STRIDE_MASK)) |
        AI_IMC_SPFCTL_ACTIVE_THRESHOLD_12 |
        AI_IMC_SPFCTL_ACTIVE_STRIDE_1;
    disabled_spfctl = enabled_spfctl &
        ~(AI_IMC_SPFCTL_ENABLE_AGT | AI_IMC_SPFCTL_ENABLE_PHT);

    TEST_SETUP_EXCEPT();
    ai_imc_write_spfctl(enabled_spfctl);
    observed_enabled = ai_imc_read_spfctl();
    const bool enabled_programmed =
        !excpt.triggered &&
        observed_enabled == enabled_spfctl;

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        warmup0 += read64(stream_base + i * 64ULL);
    }

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    TEST_SETUP_EXCEPT();
    ai_imc_write_spfctl(disabled_spfctl);
    observed_disabled = ai_imc_read_spfctl();
    const bool disabled_programmed =
        !excpt.triggered &&
        observed_disabled == disabled_spfctl;

    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        disabled0 += read64(disabled_base + i * 64ULL);
    }

    TEST_SETUP_EXCEPT();
    ai_imc_write_spfctl(enabled_spfctl);
    observed_reenabled = ai_imc_read_spfctl();
    const bool reenabled_programmed =
        !excpt.triggered &&
        observed_reenabled == enabled_spfctl;

    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        warmup1 += read64(stream_base + i * 64ULL);
        disabled1 += read64(disabled_base + i * 64ULL);
    }
    reenabled0 = read64(stream_base);
    reenabled1 = read64(stream_base + 7ULL * 64ULL);
    write64(marker, marker_value);
    fence_iorw_iorw();

    uint64_t expected_stream_sum = 0ULL;
    uint64_t expected_disabled_sum = 0ULL;
    for (uint64_t i = 0ULL; i < 8ULL; ++i) {
        expected_stream_sum += stream_seed + i;
        expected_disabled_sum += disabled_seed + i;
    }

    const bool stream_results_stay_with_explicit_loads =
        warmup0 == expected_stream_sum &&
        warmup1 == expected_stream_sum &&
        disabled0 == expected_disabled_sum &&
        disabled1 == expected_disabled_sum &&
        reenabled0 == stream_seed &&
        reenabled1 == stream_seed + 7ULL;
    const bool interrupt_between_disable_and_reenable_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool marker_and_guard_stable =
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        read64(handler_probe) == handler_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    ai_imc_write_spfctl(saved_spfctl);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("spfctl AGT/PHT enable, disable and re-enable programming is observed around the interrupt window",
        enabled_programmed && disabled_programmed && reenabled_programmed,
        "enabled=%lx/%lx disabled=%lx/%lx reenabled=%lx/%lx",
        observed_enabled, enabled_spfctl, observed_disabled,
        disabled_spfctl, observed_reenabled, enabled_spfctl);
    TEST_ASSERT("STIP between prefetch disable and re-enable is delivered once and handler reads only its probe line",
        interrupt_between_disable_and_reenable_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx probe=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_probe_value[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("post-disable and post-reenable demand load streams return their explicit payloads without stale stream or handler data",
        stream_results_stay_with_explicit_loads && marker_and_guard_stable,
        "warm=%lx/%lx disabled=%lx/%lx reenabled=%lx/%lx marker=%lx guard=%lx",
        warmup0, warmup1, disabled0, disabled1,
        reenabled0, reenabled1, read64(marker), read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_spfctl_disable_reenable_stream_owner_corner);
}

bool ai_interrupt_memory_hs_stip_replay_freelist_interrupt_flush_smoke_corner()
{
    TEST_START();

    const enum test_page base_page = VSRWX_GRWX;
    const enum test_page store_page = SWITCH1;
    const enum test_page load0_page = SWITCH2;
    const enum test_page load1_page = SCRATCHPAD;
    const uintptr_t first_off = 0x1580U;
    const uintptr_t second_off = 0x1600U;
    const uintptr_t third_off = 0x1680U;
    const uintptr_t first_base_vaddr = hs_page_base(base_page) + first_off;
    const uintptr_t second_base_vaddr = hs_page_base(base_page) + second_off;
    const uintptr_t third_base_vaddr = hs_page_base(base_page) + third_off;
    const uintptr_t first_store_vaddr = hs_page_base(store_page) + first_off;
    const uintptr_t second_store_vaddr = hs_page_base(store_page) + second_off;
    const uintptr_t third_store_vaddr = hs_page_base(store_page) + third_off;
    const uintptr_t first_load0_vaddr = hs_page_base(load0_page) + first_off;
    const uintptr_t second_load0_vaddr = hs_page_base(load0_page) + second_off;
    const uintptr_t third_load0_vaddr = hs_page_base(load0_page) + third_off;
    const uintptr_t first_load1_vaddr = hs_page_base(load1_page) + first_off;
    const uintptr_t second_load1_vaddr = hs_page_base(load1_page) + second_off;
    const uintptr_t third_load1_vaddr = hs_page_base(load1_page) + third_off;
    const uintptr_t first_paddr = phys_page_base(base_page) + first_off;
    const uintptr_t second_paddr = phys_page_base(base_page) + second_off;
    const uintptr_t third_paddr = phys_page_base(base_page) + third_off;
    const uintptr_t first_adjacent_paddr = first_paddr + sizeof(uint64_t);
    const uintptr_t second_adjacent_paddr = second_paddr + sizeof(uint64_t);
    const uintptr_t third_adjacent_paddr = third_paddr + sizeof(uint64_t);
    const uintptr_t marker = phys_page_base(base_page) + 0x1700U;
    const uint64_t first_seed = 0x696d6372666c3101ULL;
    const uint64_t second_seed = 0x696d6372666c3201ULL;
    const uint64_t third_seed = 0x696d6372666c3301ULL;
    const uint64_t first_payload = 0x696d6372666c31a1ULL;
    const uint64_t second_payload = 0x696d6372666c32b2ULL;
    const uint64_t third_payload = 0x696d6372666c33c3ULL;
    const uint64_t first_adjacent_seed = 0x696d6372666c31aaULL;
    const uint64_t second_adjacent_seed = 0x696d6372666c32aaULL;
    const uint64_t third_adjacent_seed = 0x696d6372666c33aaULL;
    const uint64_t marker_value = 0x696d6372666c6d6bULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t warmed_first = 0ULL;
    uint64_t warmed_second = 0ULL;
    uint64_t warmed_third = 0ULL;
    uint64_t first_load0 = 0ULL;
    uint64_t first_load1 = 0ULL;
    uint64_t second_load0 = 0ULL;
    uint64_t second_load1 = 0ULL;
    uint64_t third_load0 = 0ULL;
    uint64_t third_load1 = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(first_paddr, first_seed);
    write64(second_paddr, second_seed);
    write64(third_paddr, third_seed);
    write64(first_adjacent_paddr, first_adjacent_seed);
    write64(second_adjacent_paddr, second_adjacent_seed);
    write64(third_adjacent_paddr, third_adjacent_seed);
    write64(marker, 0ULL);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(base_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    ai_imc_write_hs_leaf_to_page(store_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    ai_imc_write_hs_leaf_to_page(load0_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    ai_imc_write_hs_leaf_to_page(load1_page, base_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRW(CSR_MENVCFG, (saved_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed_first = ai_imc_ld_probe(first_base_vaddr, 0ULL);
    warmed_second = ai_imc_ld_probe(second_base_vaddr, 0ULL);
    warmed_third = ai_imc_ld_probe(third_base_vaddr, 0ULL);
    const bool warmup_ok =
        !excpt.triggered &&
        warmed_first == first_seed &&
        warmed_second == second_seed &&
        warmed_third == third_seed;

    TEST_SETUP_EXCEPT();
    cbo_inval(first_base_vaddr);
    fence_iorw_iorw();
    ai_imc_replay_pair_capture(first_store_vaddr, first_load0_vaddr,
        first_load1_vaddr, first_payload, &first_load0, &first_load1);
    const bool first_episode_ok =
        !excpt.triggered &&
        first_load0 == first_payload &&
        first_load1 == first_payload;

    goto_priv(PRIV_M);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    cbo_inval(second_base_vaddr);
    fence_iorw_iorw();
    ai_imc_replay_pair_capture(second_store_vaddr, second_load0_vaddr,
        second_load1_vaddr, second_payload, &second_load0, &second_load1);
    cbo_inval(third_base_vaddr);
    fence_iorw_iorw();
    ai_imc_replay_pair_capture(third_store_vaddr, third_load0_vaddr,
        third_load1_vaddr, third_payload, &third_load0, &third_load1);
    write64(marker, marker_value);
    fence_iorw_iorw();
    const bool post_interrupt_episodes_ok =
        !excpt.triggered &&
        second_load0 == second_payload &&
        second_load1 == second_payload &&
        third_load0 == third_payload &&
        third_load1 == third_payload;

    goto_priv(PRIV_M);
    const bool interrupt_flush_boundary_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool final_images_no_stale_slot =
        read64(first_paddr) == first_payload &&
        read64(second_paddr) == second_payload &&
        read64(third_paddr) == third_payload &&
        read64(first_adjacent_paddr) == first_adjacent_seed &&
        read64(second_adjacent_paddr) == second_adjacent_seed &&
        read64(third_adjacent_paddr) == third_adjacent_seed &&
        read64(marker) == marker_value &&
        first_payload != second_payload &&
        second_payload != third_payload &&
        third_payload != ai_imc_handler_log_page[0];

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("three same-PA alias replay-pressure targets are warmed before the interrupt flush boundary",
        warmup_ok,
        "warm=%lx,%lx,%lx seed=%lx,%lx,%lx",
        warmed_first, warmed_second, warmed_third,
        first_seed, second_seed, third_seed);
    TEST_ASSERT("the first replay-pressure episode before STIP produces one coherent payload on both younger aliases",
        first_episode_ok,
        "l0=%lx l1=%lx expected=%lx",
        first_load0, first_load1, first_payload);
    TEST_ASSERT("STIP between replay-pressure episodes is delivered once and clears pending state before post-return allocations",
        interrupt_flush_boundary_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0], CSRR(CSR_MIP));
    TEST_ASSERT("post-interrupt replay-pressure episodes allocate fresh owners and do not reuse stale payloads or duplicate a previous result",
        post_interrupt_episodes_ok && final_images_no_stale_slot,
        "second=%lx,%lx third=%lx,%lx backing=%lx,%lx,%lx marker=%lx",
        second_load0, second_load1, third_load0, third_load1,
        read64(first_paddr), read64(second_paddr), read64(third_paddr),
        read64(marker));

    TEST_END(ai_interrupt_memory_hs_stip_replay_freelist_interrupt_flush_smoke_corner);
}

bool ai_interrupt_memory_hs_ssip_fast_replay_vector_priority_progress_smoke_corner()
{
    TEST_START();

    const enum test_page cold_page = VSRWX_GRWX;
    const enum test_page vec_src_page = SWITCH1;
    const enum test_page vec_dst_page = SWITCH2;
    const enum test_page pressure_page = SCRATCHPAD;
    const uintptr_t cold_vaddr = hs_page_base(cold_page) + 0x17c0U;
    const uintptr_t cold_paddr = phys_page_base(cold_page) + 0x17c0U;
    const uintptr_t vec_src_vaddr = hs_page_base(vec_src_page) + 0x1780U;
    const uintptr_t vec_src_paddr = phys_page_base(vec_src_page) + 0x1780U;
    const uintptr_t vec_dst_vaddr = hs_page_base(vec_dst_page) + 0x1780U;
    const uintptr_t vec_dst_paddr = phys_page_base(vec_dst_page) + 0x1780U;
    const uintptr_t pressure_vaddr = hs_page_base(pressure_page) + 0x1780U;
    const uintptr_t pressure_paddr = phys_page_base(pressure_page) + 0x1780U;
    const uintptr_t marker_vaddr = hs_page_base(cold_page) + 0x17d0U;
    const uintptr_t marker_paddr = phys_page_base(cold_page) + 0x17d0U;
    const uintptr_t guard_paddr = phys_page_base(cold_page) + 0x17e8U;
    const uint64_t cold_seed = 0x696d636672707331ULL;
    const uint64_t cold_fresh = 0x696d636672707332ULL;
    const uint64_t pressure0 = 0x696d636672707370ULL;
    const uint64_t pressure1 = 0x3535353535353535ULL;
    const uint64_t pressure_expected = pressure0 ^ pressure1;
    const uint64_t marker_seed = 0x696d63667270736dULL;
    const uint64_t marker_value = 0x696d63667270736eULL;
    const uint64_t guard_seed = 0x696d6366727073aaULL;
    const uint8_t vec_pattern[AI_IMC_FAST_REPLAY_VEC_BYTES] = {
        0x31U, 0x42U, 0x53U, 0x64U, 0x75U, 0x86U, 0x97U, 0xa8U,
        0xb9U, 0xcaU, 0xdbU, 0xecU, 0xfdU, 0x0eU, 0x1fU, 0x20U
    };
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    uint64_t warmed = 0ULL;
    uint64_t cold_observed = 0ULL;
    uint64_t pressure_mix = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    CSRW(CSR_MENVCFG, (saved_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);
    write64(cold_paddr, cold_seed);
    write64(marker_paddr, marker_seed);
    write64(marker_paddr + sizeof(uint64_t), 0ULL);
    write64(guard_paddr, guard_seed);
    write64(pressure_paddr, pressure0);
    write64(pressure_paddr + sizeof(uint64_t), pressure1);
    ai_imc_handler_probe_addr = pressure_vaddr;
    ai_imc_write_bytes(vec_src_paddr, vec_pattern,
        AI_IMC_FAST_REPLAY_VEC_BYTES);
    for (uint64_t i = 0ULL; i < AI_IMC_FAST_REPLAY_VEC_BYTES; ++i) {
        write8(vec_dst_paddr + i, 0xa5U);
    }
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    goto_priv(PRIV_HS);
    hspt_init();
    CSRW(CSR_STVEC, (uintptr_t)ai_imc_stvec_same_target_load_handler);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    sfence_vma();

    TEST_SETUP_EXCEPT();
    warmed = ai_imc_ld_probe(cold_vaddr, 0ULL);
    const bool cold_line_warmed =
        !excpt.triggered &&
        warmed == cold_seed;

    TEST_SETUP_EXCEPT();
    cbo_inval(cold_vaddr);
    const bool cold_line_invalidated = !excpt.triggered;

    goto_priv(PRIV_M);
    write64(cold_paddr, cold_fresh);
    fence_iorw_iorw();
    ai_imc_reset_trap_log();
    ai_imc_prepare_hs_ssip(1ULL);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    ai_imc_fast_replay_vector_priority_window(cold_vaddr, vec_src_vaddr,
        vec_dst_vaddr, pressure_vaddr, marker_vaddr, marker_value,
        &cold_observed, &pressure_mix);
    const bool mixed_window_no_exception = !excpt.triggered;

    goto_priv(PRIV_M);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    fence_iorw_iorw();

    const bool interrupt_delivered_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_SSI &&
        (ai_imc_trap_mip[0] & AI_IMC_SIP_SSIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == pressure0 &&
        ai_imc_handler_log_page[0] == 0x696d6373686c6700ULL &&
        (CSRR(CSR_SIP) & AI_IMC_SIP_SSIP) == 0ULL;
    const bool all_memory_sources_progress =
        mixed_window_no_exception &&
        cold_observed == cold_fresh &&
        pressure_mix == pressure_expected &&
        ai_imc_bytes_match(vec_dst_paddr, vec_pattern,
            AI_IMC_FAST_REPLAY_VEC_BYTES) &&
        read64(marker_paddr) == marker_value &&
        read64(marker_paddr + sizeof(uint64_t)) == pressure_expected &&
        read64(guard_paddr) == guard_seed &&
        read64(cold_paddr) == cold_fresh &&
        read64(pressure_paddr) == pressure0 &&
        read64(pressure_paddr + sizeof(uint64_t)) == pressure1;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_VSTART, 0ULL);
    ai_imc_handler_probe_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRC(CSR_SIE, AI_IMC_SIE_SSIE);
    CSRC(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("the scalar cold-load candidate is warmed then invalidated before the interrupt-adjacent mixed replay window",
        cold_line_warmed && cold_line_invalidated,
        "warm=%lx seed=%lx inval_ok=%d excpt=%d cause=%lx",
        warmed, cold_seed, cold_line_invalidated, excpt.triggered,
        excpt.cause);
    TEST_ASSERT("delegated SSIP delivered from the mixed scalar/vector/pressure window is observed once and clears its source",
        interrupt_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx sip=%lx probe=%lx final_sip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_probe_value[0],
        CSRR(CSR_SIP));
    TEST_ASSERT("post-interrupt scalar cold-load, vector copy, pressure load and marker results all complete without stale or duplicate owner symptoms",
        all_memory_sources_progress,
        "cold=%lx expected=%lx pressure=%lx expectedp=%lx marker=%lx mixslot=%lx guard=%lx vec0=%x vec15=%x",
        cold_observed, cold_fresh, pressure_mix, pressure_expected,
        read64(marker_paddr), read64(marker_paddr + sizeof(uint64_t)),
        read64(guard_paddr), read8(vec_dst_paddr),
        read8(vec_dst_paddr + AI_IMC_FAST_REPLAY_VEC_BYTES - 1U));

    TEST_END(ai_interrupt_memory_hs_ssip_fast_replay_vector_priority_progress_smoke_corner);
}

bool ai_interrupt_memory_m_stip_skipped_partial_store_post_handler_mask_corner()
{
    TEST_START();

    static const uint8_t seed[16] = {
        0x20U, 0x31U, 0x42U, 0x53U,
        0x64U, 0x75U, 0x86U, 0x97U,
        0xa8U, 0xb9U, 0xcaU, 0xdbU,
        0xecU, 0xfdU, 0x0eU, 0x1fU,
    };
    static const uint8_t expected[16] = {
        0x20U, 0xa1U, 0x42U, 0x53U,
        0xc3U, 0xb2U, 0x86U, 0x97U,
        0x07U, 0xf6U, 0xe5U, 0xd4U,
        0xecU, 0xfdU, 0x0eU, 0x1fU,
    };
    const uintptr_t base = phys_page_base(SWITCH2) + 0x1d00U;
    const uintptr_t marker = phys_page_base(SWITCH2) + 0x1d40U;
    const uintptr_t guard = phys_page_base(SWITCH2) + 0x1d48U;
    const uint64_t skipped_byte = 0xe1ULL;
    const uint64_t skipped_half = 0xe2f3ULL;
    const uint64_t skipped_word = 0xe4f50617ULL;
    const uint64_t marker_value = 0x696d63733131314dULL;
    const uint64_t guard_seed = 0x696d637331313147ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t rounds = 0ULL;
    uint64_t bad_index = 0ULL;

    goto_priv(PRIV_M);
    ai_imc_write_bytes(base, seed, sizeof(seed));
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = base;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "li t0, 1\n\t"
        "bnez t0, 1f\n\t"
        "sb %[skipped_byte], 1(%[base])\n\t"
        "sh %[skipped_half], 4(%[base])\n\t"
        "sw %[skipped_word], 8(%[base])\n\t"
        "1:\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        ".option pop\n\t"
        :
        : [base] "r"(base),
          [skipped_byte] "r"(skipped_byte),
          [skipped_half] "r"(skipped_half),
          [skipped_word] "r"(skipped_word),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "t0", "memory");
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);

    const bool skipped_partial_store_has_no_side_effect =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == read64(base) &&
        ai_imc_bytes_match_array(base, seed, sizeof(seed), &bad_index) &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    sb(base + 1U, 0xa1U);
    sh(base + 4U, 0xb2c3U);
    sw(base + 8U, 0xd4e5f607U);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool post_handler_store_uses_fresh_mask =
        ai_imc_bytes_match_array(base, expected, sizeof(expected),
            &bad_index) &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        ai_imc_handler_log_page[1] == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("branch-skipped partial stores leave no byte-valid side effect before the pending STIP handler probes the target",
        skipped_partial_store_has_no_side_effect,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx line0=%lx bad_index=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], read64(base), bad_index,
        CSRR(CSR_MIP));
    TEST_ASSERT("the first post-handler partial stores use fresh byte masks and do not inherit skipped-store metadata",
        post_handler_store_uses_fresh_mask,
        "bad_index=%lx line0=%lx line1=%lx marker=%lx guard=%lx log0=%lx log1=%lx",
        bad_index, read64(base), read64(base + 8U), read64(marker),
        read64(guard), ai_imc_handler_log_page[0],
        ai_imc_handler_log_page[1]);

    TEST_END(ai_interrupt_memory_m_stip_skipped_partial_store_post_handler_mask_corner);
}

bool ai_interrupt_memory_m_stip_wrong_path_load_result_quarantine_corner()
{
    TEST_START();

    const uintptr_t good_addr = phys_page_base(SWITCH1) + 0x1d00U;
    const uintptr_t wrong_addr = phys_page_base(SWITCH1) + 0x1d08U;
    const uintptr_t handler_probe = phys_page_base(SWITCH1) + 0x1d10U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1d18U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1d20U;
    const uint64_t good_value = 0x696d63313132676fULL;
    const uint64_t wrong_value = 0x696d633131327772ULL;
    const uint64_t handler_seed = 0x696d633131326870ULL;
    const uint64_t marker_value = 0x696d633131326d6bULL;
    const uint64_t guard_seed = 0x696d633131326775ULL;
    const uint64_t sentinel = 0x696d633131327365ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t skipped_load_result = sentinel;
    uint64_t correct_load_result = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(good_addr, good_value);
    write64(wrong_addr, wrong_value);
    write64(handler_probe, handler_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_target_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    asm volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "mv %[result], %[sentinel]\n\t"
        "li t0, 1\n\t"
        "bnez t0, 1f\n\t"
        "ld %[result], 0(%[wrong_addr])\n\t"
        "1:\n\t"
        "csrs mip, %[stip]\n\t"
        "csrs mstatus, %[mie]\n\t"
        ".option pop\n\t"
        : [result] "+r"(skipped_load_result)
        : [wrong_addr] "r"(wrong_addr),
          [sentinel] "r"(sentinel),
          [stip] "r"(AI_IMC_MIP_STIP),
          [mie] "r"(MSTATUS_MIE)
        : "t0", "memory");
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    correct_load_result = read64(good_addr);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool wrong_path_load_result_quarantined =
        skipped_load_result == sentinel &&
        skipped_load_result != wrong_value &&
        correct_load_result == good_value &&
        read64(wrong_addr) == wrong_value;
    const bool interrupt_handler_load_and_marker_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        ai_imc_handler_probe_value[0] != good_value &&
        ai_imc_handler_probe_value[0] != wrong_value &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("branch-skipped wrong-address load keeps the destination sentinel and the post-handler correct load reads the good line",
        wrong_path_load_result_quarantined,
        "skipped=%lx sentinel=%lx wrong=%lx correct=%lx good=%lx",
        skipped_load_result, sentinel, wrong_value,
        correct_load_result, good_value);
    TEST_ASSERT("the STIP handler load observes only its own probe line and clears source state without poisoning main load results",
        interrupt_handler_load_and_marker_isolated,
        "rounds=%lx trap_count=%lx cause=%lx probe=%lx marker=%lx guard=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_handler_probe_value[0], read64(marker), read64(guard),
        CSRR(CSR_MIP));

    TEST_END(ai_interrupt_memory_m_stip_wrong_path_load_result_quarantine_corner);
}

bool ai_interrupt_memory_m_stip_same_pc_helper_load_trace_identity_smoke_corner()
{
    TEST_START();

    const uintptr_t main_a = phys_page_base(SWITCH1) + 0x1d80U;
    const uintptr_t handler_probe = phys_page_base(SWITCH1) + 0x1d88U;
    const uintptr_t main_b = phys_page_base(SWITCH1) + 0x1d90U;
    const uintptr_t marker = phys_page_base(SWITCH1) + 0x1d98U;
    const uintptr_t guard = phys_page_base(SWITCH1) + 0x1da0U;
    const uint64_t main_a_seed = 0x696d637370686141ULL;
    const uint64_t handler_seed = 0x696d637370686848ULL;
    const uint64_t main_b_seed = 0x696d637370686242ULL;
    const uint64_t marker_value = 0x696d637370686d21ULL;
    const uint64_t guard_seed = 0x696d637370686747ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t main_a_value = 0ULL;
    uint64_t main_b_value = 0ULL;
    uint64_t rounds = 0ULL;
    volatile uintptr_t main_a_pc = 0ULL;
    volatile uintptr_t main_b_pc = 0ULL;

    goto_priv(PRIV_M);
    write64(main_a, main_a_seed);
    write64(handler_probe, handler_seed);
    write64(main_b, main_b_seed);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = handler_probe;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_same_pc_helper_load_handler);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);

    main_a_value = ai_imc_same_pc_load_helper(main_a, &main_a_pc);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    main_b_value = ai_imc_same_pc_load_helper(main_b, &main_b_pc);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool same_helper_pc_reused_without_result_alias =
        main_a_pc != 0ULL &&
        main_a_pc == main_b_pc &&
        main_a_pc == ai_imc_same_pc_helper_pc[0] &&
        main_a_value == main_a_seed &&
        ai_imc_handler_probe_value[0] == handler_seed &&
        main_b_value == main_b_seed &&
        main_a_value != ai_imc_handler_probe_value[0] &&
        main_b_value != ai_imc_handler_probe_value[0];
    const bool interrupt_entry_and_memory_episodes_isolated =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        read64(main_a) == main_a_seed &&
        read64(handler_probe) == handler_seed &&
        read64(main_b) == main_b_seed &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_handler_probe_addr = 0ULL;
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("mainline and handler loads reuse the same helper load PC but keep address-specific results",
        same_helper_pc_reused_without_result_alias,
        "pcA=%lx pcH=%lx pcB=%lx valA=%lx valH=%lx valB=%lx",
        (uint64_t)main_a_pc, (uint64_t)ai_imc_same_pc_helper_pc[0],
        (uint64_t)main_b_pc, main_a_value,
        ai_imc_handler_probe_value[0], main_b_value);
    TEST_ASSERT("the STIP entry, source clear, marker store and three load episodes stay in separate owners",
        interrupt_entry_and_memory_episodes_isolated,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx marker=%lx guard=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], read64(marker), read64(guard),
        CSRR(CSR_MIP));

    TEST_END(ai_interrupt_memory_m_stip_same_pc_helper_load_trace_identity_smoke_corner);
}

bool ai_interrupt_memory_m_stip_pmp_4kb_split_retry_owner_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const uintptr_t split_vaddr = hs_page_base(low_page) + PAGE_SIZE -
        sizeof(uint32_t);
    const uintptr_t second_half_vaddr = hs_page_base(high_page);
    const uintptr_t low_tail_paddr = phys_page_base(low_page) + PAGE_SIZE -
        sizeof(uint64_t);
    const uintptr_t high_head_paddr = phys_page_base(high_page);
    const uintptr_t low_guard = low_tail_paddr - sizeof(uint64_t);
    const uintptr_t high_guard = high_head_paddr + sizeof(uint64_t);
    const uintptr_t high_page_base = high_head_paddr & PAGE_ADDR_MSK;
    const uintptr_t high_page_end = high_page_base + PAGE_SIZE;
    const uint64_t low_seed = 0x1021324354657687ULL;
    const uint64_t high_seed = 0x98a9bacbdcedfe0fULL;
    const uint64_t fault_payload = 0x0123456789abcdefULL;
    const uint64_t retry_payload = 0xf0e1d2c3b4a59687ULL;
    const uint64_t low_guard_seed = 0x696d633139376c67ULL;
    const uint64_t high_guard_seed = 0x696d633139376867ULL;
    const uint64_t expected_load =
        ((high_seed & 0xffffffffULL) << 32U) | (low_seed >> 32U);
    const uint64_t allowed_fault_prefix_low =
        (low_seed & 0xffffffffULL) |
        ((fault_payload & 0xffffffffULL) << 32U);
    const uint64_t expected_retry_low =
        (low_seed & 0xffffffffULL) |
        ((retry_payload & 0xffffffffULL) << 32U);
    const uint64_t expected_retry_high =
        (high_seed & 0xffffffff00000000ULL) |
        (retry_payload >> 32U);
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
    uint64_t baseline_load = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pmp_deny_cfg = 0ULL;
    uint64_t pmp_allow_cfg = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(low_guard, low_guard_seed);
    write64(low_tail_paddr, low_seed);
    write64(high_head_paddr, high_seed);
    write64(high_guard, high_guard_seed);
    fence_iorw_iorw();

    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    baseline_load = ld(split_vaddr);
    ai_imc_clear_mprv_translation();
    const bool baseline_split_load_ok =
        !excpt.triggered &&
        baseline_load == expected_load &&
        read64(low_guard) == low_guard_seed &&
        read64(low_tail_paddr) == low_seed &&
        read64(high_head_paddr) == high_seed &&
        read64(high_guard) == high_guard_seed;

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(high_page_base, high_page_end,
        AI_IMC_PMP_TOR_NO_W);
    pmp_deny_cfg = CSRR(CSR_PMPCFG0);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, fault_payload);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    pending_after_fault = CSRR(CSR_MIP);
    const bool low_half_prefix_allowed =
        read64(low_tail_paddr) == low_seed ||
        read64(low_tail_paddr) == allowed_fault_prefix_low;
    const bool high_half_pmp_fault_contract =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        fault_cause == CAUSE_SAF &&
        fault_tval == second_half_vaddr &&
        pmp_deny_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_NO_W) &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_trap_count == 0ULL &&
        low_half_prefix_allowed &&
        read64(high_head_paddr) == high_seed &&
        read64(low_guard) == low_guard_seed &&
        read64(high_guard) == high_guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_after_split_fault_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    ai_imc_configure_pmp_window(high_page_base, high_page_end,
        AI_IMC_PMP_TOR_RWX);
    pmp_allow_cfg = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(split_vaddr, retry_payload);
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();
    const bool retry_uses_fresh_pmp_generation =
        !excpt.triggered &&
        pmp_allow_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_RWX) &&
        read64(low_tail_paddr) == expected_retry_low &&
        read64(high_head_paddr) == expected_retry_high &&
        read64(low_tail_paddr) != allowed_fault_prefix_low &&
        read64(low_guard) == low_guard_seed &&
        read64(high_guard) == high_guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("baseline 4KB-boundary split load stitches low/high halves before PMP deny is installed",
        baseline_split_load_ok,
        "load=%lx expected=%lx low=%lx high=%lx guards=%lx,%lx",
        baseline_load, expected_load, read64(low_tail_paddr),
        read64(high_head_paddr), read64(low_guard), read64(high_guard));
    TEST_ASSERT("split store high-half PMP deny reports store access fault on the second half and leaves STIP pending",
        high_half_pmp_fault_contract,
        "cause=%lx tval=%lx expected_tval=%lx cfg=%lx pending=%lx low=%lx prefix=%lx high=%lx",
        fault_cause, fault_tval, second_half_vaddr, pmp_deny_cfg,
        pending_after_fault, read64(low_tail_paddr),
        allowed_fault_prefix_low, read64(high_head_paddr));
    TEST_ASSERT("the preserved STIP after the PMP split fault is delivered exactly once before retry",
        interrupt_after_split_fault_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("after repairing the high-half PMP window, retry store writes one fresh payload across both halves",
        retry_uses_fresh_pmp_generation,
        "cfg=%lx low=%lx expected_low=%lx high=%lx expected_high=%lx guards=%lx,%lx",
        pmp_allow_cfg, read64(low_tail_paddr), expected_retry_low,
        read64(high_head_paddr), expected_retry_high,
        read64(low_guard), read64(high_guard));

    TEST_END(ai_interrupt_memory_m_stip_pmp_4kb_split_retry_owner_corner);
}

bool ai_interrupt_memory_m_stip_pmpcfg_partial_update_interrupt_snapshot_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x1c80U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x1c80U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t page_base = paddr & PAGE_ADDR_MSK;
    const uintptr_t page_end = page_base + PAGE_SIZE;
    const uint64_t seed = 0x696d633139387365ULL;
    const uint64_t allow0_payload = 0x696d633139384130ULL;
    const uint64_t deny_payload = 0x696d633139386464ULL;
    const uint64_t allow1_payload = 0x696d633139384131ULL;
    const uint64_t guard_seed = 0x696d633139386775ULL;
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
    uint64_t partial_addr_cfg = 0ULL;
    uint64_t deny_cfg = 0ULL;
    uint64_t allow_cfg = 0ULL;
    uint64_t partial_access_cause = 0ULL;
    uint64_t partial_access_tval = 0ULL;
    uint64_t deny_cause = 0ULL;
    uint64_t deny_tval = 0ULL;
    uint64_t pending_before_interrupt = 0ULL;
    uint64_t rounds = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();
    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allow0_payload);
    ai_imc_clear_mprv_translation();
    const bool initial_full_allow_works =
        !excpt.triggered &&
        read64(paddr) == allow0_payload &&
        read64(guard) == guard_seed;

    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, page_base >> 2U);
    CSRW(CSR_PMPADDR1, page_end >> 2U);
    partial_addr_cfg = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, deny_payload);
    ai_imc_clear_mprv_translation();
    partial_access_cause = excpt.cause;
    partial_access_tval = excpt.tval;
    const bool partial_addr_update_not_mixed_with_old_allow =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        partial_access_cause == CAUSE_SAF &&
        partial_access_tval == vaddr &&
        partial_addr_cfg == 0ULL &&
        read64(paddr) == allow0_payload &&
        read64(guard) == guard_seed;

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_PMPCFG0, ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_NO_W));
    deny_cfg = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, deny_payload);
    ai_imc_clear_mprv_translation();
    deny_cause = excpt.cause;
    deny_tval = excpt.tval;
    pending_before_interrupt = CSRR(CSR_MIP);
    const bool full_deny_snapshot_blocks_store =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        deny_cause == CAUSE_SAF &&
        deny_tval == vaddr &&
        deny_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_NO_W) &&
        (pending_before_interrupt & AI_IMC_MIP_STIP) != 0ULL &&
        read64(paddr) == allow0_payload &&
        read64(guard) == guard_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_between_pmpcfg_generations_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;

    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, page_base >> 2U);
    CSRW(CSR_PMPADDR1, page_end >> 2U);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_RWX));
    allow_cfg = CSRR(CSR_PMPCFG0);
    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(vaddr, allow1_payload);
    ai_imc_clear_mprv_translation();
    fence_iorw_iorw();
    const bool final_allow_snapshot_commits_once =
        !excpt.triggered &&
        allow_cfg == ai_imc_pmp_window_cfg(AI_IMC_PMP_TOR_RWX) &&
        read64(paddr) == allow1_payload &&
        read64(paddr) != deny_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("initial full PMP allow snapshot permits the baseline translated store",
        initial_full_allow_works,
        "target=%lx guard=%lx", read64(paddr), read64(guard));
    TEST_ASSERT("after writing PMP address bounds but before pmpcfg, target access does not inherit the previous allow permission",
        partial_addr_update_not_mixed_with_old_allow,
        "cause=%lx tval=%lx expected=%lx cfg=%lx target=%lx guard=%lx",
        partial_access_cause, partial_access_tval, vaddr,
        partial_addr_cfg, read64(paddr), read64(guard));
    TEST_ASSERT("the complete PMP deny snapshot blocks the store while preserving the pending STIP",
        full_deny_snapshot_blocks_store,
        "cause=%lx tval=%lx cfg=%lx pending=%lx target=%lx guard=%lx",
        deny_cause, deny_tval, deny_cfg, pending_before_interrupt,
        read64(paddr), read64(guard));
    TEST_ASSERT("STIP delivered between PMP config generations logs once and clears source state",
        interrupt_between_pmpcfg_generations_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("after rewriting a complete allow snapshot, retry store uses only the final permission and commits once",
        final_allow_snapshot_commits_once,
        "cfg=%lx target=%lx denied=%lx guard=%lx",
        allow_cfg, read64(paddr), deny_payload, read64(guard));

    TEST_END(ai_interrupt_memory_m_stip_pmpcfg_partial_update_interrupt_snapshot_corner);
}

bool ai_interrupt_memory_m_stip_pma_cache_atomic_retry_generation_corner()
{
    TEST_START();

    const enum test_page target_page = SWITCH2;
    const uintptr_t paddr = phys_page_base(target_page) + 0x1d80U;
    const uintptr_t guard = paddr + sizeof(uint64_t);
    const uintptr_t pma_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pma_end = pma_start + PAGE_SIZE;
    const uint64_t seed = 0x696d633139397365ULL;
    const uint64_t baseline_addend = 0x11ULL;
    const uint64_t fault_addend = 0x22ULL;
    const uint64_t retry_addend = 0x33ULL;
    const uint64_t guard_seed = 0x696d633139396775ULL;
    const uint64_t amo_fault_sentinel = 0x696d633139397264ULL;
    const uint64_t allow_cfg = ai_imc_pma_tor_entry1_cfg(
        AI_IMC_PMA_R | AI_IMC_PMA_W | AI_IMC_PMA_X |
        AI_IMC_PMA_A_TOR | AI_IMC_PMA_ATOMIC | AI_IMC_PMA_C);
    const uint64_t atomic_off_cfg = ai_imc_pma_tor_entry1_cfg(
        AI_IMC_PMA_R | AI_IMC_PMA_W | AI_IMC_PMA_X |
        AI_IMC_PMA_A_TOR | AI_IMC_PMA_C);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_pmacfg0 = CSRR(CSR_PMACFG0);
    const uint64_t saved_pmacfg2 = CSRR(CSR_PMACFG2);
    const uint64_t saved_pmaaddr0 = CSRR(CSR_PMAADDR0);
    const uint64_t saved_pmaaddr1 = CSRR(CSR_PMAADDR1);
    const uint64_t saved_pmaaddr12 = CSRR(CSR_PMAADDR12);
    const uint64_t saved_pmaaddr13 = CSRR(CSR_PMAADDR13);
    const uint64_t saved_pmaaddr14 = CSRR(CSR_PMAADDR14);
    const uint64_t saved_pmaaddr15 = CSRR(CSR_PMAADDR15);
    uint64_t baseline_old = 0ULL;
    uint64_t fault_old = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;

    goto_priv(PRIV_M);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMACFG2, 0ULL);
    CSRW(CSR_PMAADDR0, pma_start >> 2U);
    CSRW(CSR_PMAADDR1, pma_end >> 2U);
    CSRW(CSR_PMACFG0, allow_cfg);
    fence_iorw_iorw();
    TEST_SETUP_EXCEPT();
    baseline_old = ai_imc_amoadd_d((uint64_t *)paddr, baseline_addend);
    const bool cacheable_atomic_on_allows_amo =
        !excpt.triggered &&
        baseline_old == seed &&
        read64(paddr) == seed + baseline_addend &&
        read64(guard) == guard_seed;

    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, pma_start >> 2U);
    CSRW(CSR_PMAADDR1, pma_end >> 2U);
    CSRW(CSR_PMACFG0, atomic_off_cfg);
    fence_iorw_iorw();
    TEST_SETUP_EXCEPT();
    fault_old = ai_imc_amoadd_d_addr(paddr, fault_addend,
        amo_fault_sentinel);
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    const bool cacheable_atomic_off_blocks_amo =
        excpt.triggered &&
        fault_cause == CAUSE_SAF &&
        fault_tval == paddr &&
        fault_old == amo_fault_sentinel &&
        read64(paddr) == seed + baseline_addend &&
        read64(guard) == guard_seed;

    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_pma_retry_target_addr = paddr;
    ai_imc_pma_retry_pmaaddr0 = pma_start >> 2U;
    ai_imc_pma_retry_pmaaddr1 = pma_end >> 2U;
    ai_imc_pma_retry_allow_cfg = allow_cfg;
    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, pma_start >> 2U);
    CSRW(CSR_PMAADDR1, pma_end >> 2U);
    CSRW(CSR_PMACFG0, atomic_off_cfg);
    fence_iorw_iorw();
    CSRW(CSR_MTVEC,
        (uintptr_t)ai_imc_mtvec_pma_atomic_retry_generation_handler);
    TEST_SETUP_EXCEPT();
    const uint64_t retry_old = ai_imc_amoadd_d_addr(paddr, retry_addend,
        amo_fault_sentinel);
    const uint64_t retry_final = read64(paddr);
    const uint64_t trap_count = ai_imc_trap_count;
    const uint64_t repair_count = ai_imc_pma_retry_fault_repairs;
    const uint64_t interrupt_count = ai_imc_pma_retry_interrupts;
    const uint64_t final_pmacfg0 = CSRR(CSR_PMACFG0);
    const uint64_t final_mip = CSRR(CSR_MIP);
    const bool retry_uses_full_repaired_pma_generation =
        !excpt.triggered &&
        trap_count == 2ULL &&
        ai_imc_trap_cause[0] == CAUSE_SAF &&
        ai_imc_trap_tval[0] == paddr &&
        ai_imc_trap_cause[1] == CAUSE_STI &&
        (ai_imc_trap_mip[1] & AI_IMC_MIP_STIP) != 0ULL &&
        repair_count == 1ULL &&
        interrupt_count == 1ULL &&
        ai_imc_pma_retry_value_on_fault == seed + baseline_addend &&
        ai_imc_pma_retry_pmacfg_seen_in_interrupt == allow_cfg &&
        ai_imc_pma_retry_target_after_complete ==
            seed + baseline_addend &&
        retry_old == seed + baseline_addend &&
        retry_final == seed + baseline_addend + retry_addend &&
        read64(guard) == guard_seed &&
        final_pmacfg0 == allow_cfg &&
        (final_mip & AI_IMC_MIP_STIP) == 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_PMACFG0, 0ULL);
    CSRW(CSR_PMAADDR0, saved_pmaaddr0);
    CSRW(CSR_PMAADDR1, saved_pmaaddr1);
    CSRW(CSR_PMAADDR12, saved_pmaaddr12);
    CSRW(CSR_PMAADDR13, saved_pmaaddr13);
    CSRW(CSR_PMAADDR14, saved_pmaaddr14);
    CSRW(CSR_PMAADDR15, saved_pmaaddr15);
    CSRW(CSR_PMACFG2, saved_pmacfg2);
    CSRW(CSR_PMACFG0, saved_pmacfg0);
    fence_iorw_iorw();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("profile contract baseline: cacheable memory with PMA Atomic set accepts AMO and commits one RMW",
        cacheable_atomic_on_allows_amo,
        "old=%lx target=%lx guard=%lx cfg=%lx",
        baseline_old, read64(paddr), read64(guard), allow_cfg);
    TEST_ASSERT("profile contract negative: cacheable memory with PMA Atomic clear faults AMO without side effect",
        cacheable_atomic_off_blocks_amo,
        "cause=%lx tval=%lx old=%lx target=%lx guard=%lx cfg=%lx",
        fault_cause, fault_tval, fault_old, read64(paddr),
        read64(guard), atomic_off_cfg);
    TEST_ASSERT("after PMA repair and intervening STIP, AMO retry uses the complete cacheable+Atomic generation exactly once",
        retry_uses_full_repaired_pma_generation,
        "trap_count=%lx repair=%lx intr=%lx c0=%lx tv0=%lx c1=%lx pmacfg_seen=%lx final_cfg=%lx old=%lx target=%lx fault_value=%lx intr_value=%lx mip=%lx",
        trap_count, repair_count, interrupt_count,
        ai_imc_trap_cause[0], ai_imc_trap_tval[0],
        ai_imc_trap_cause[1],
        ai_imc_pma_retry_pmacfg_seen_in_interrupt, final_pmacfg0,
        retry_old, retry_final, ai_imc_pma_retry_value_on_fault,
        ai_imc_pma_retry_target_after_complete, final_mip);

    TEST_END(ai_interrupt_memory_m_stip_pma_cache_atomic_retry_generation_corner);
}

bool ai_interrupt_memory_hs_stip_high_ppn_fault_retry_priority_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const enum test_page guard_page = SWITCH2;
    const uintptr_t vaddr = hs_page_base(target_page) + 0x1d80U;
    const uintptr_t paddr = phys_page_base(target_page) + 0x1d80U;
    const uintptr_t guard = phys_page_base(guard_page) + 0x1d80U;
    const uint64_t seed = 0x696d633135377365ULL;
    const uint64_t store_payload = 0x696d633135377374ULL;
    const uint64_t guard_seed = 0x696d633135376775ULL;
    const uint64_t load_sentinel = 0x696d63313537736eULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    pte_t *leaf = NULL;
    pte_t high_ppn_leaf = 0ULL;
    pte_t fixed_leaf = 0ULL;
    pte_t leaf_after_fault = 0ULL;
    uint64_t denied_load = load_sentinel;
    uint64_t fault_priv = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t retry_load = 0ULL;
    uint64_t retry_readback = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t final_mip = 0ULL;
    pte_t final_leaf = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(paddr, seed);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    goto_priv(PRIV_M);
    leaf = ai_imc_hs_leaf_entry(target_page);
    high_ppn_leaf = (phys_page_base(target_page) >> 2U) |
        PTE_V | PTE_RWX | (1ULL << 46);
    fixed_leaf = (phys_page_base(target_page) >> 2U) |
        PTE_V | PTE_AD | PTE_RWX;
    *leaf = high_ppn_leaf;
    sfence_vma();

    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    denied_load = ai_imc_ld_probe(vaddr, load_sentinel);
    fault_priv = excpt.priv;
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    leaf_after_fault = *leaf;
    pending_after_fault = CSRR(CSR_MIP);
    const bool high_ppn_fault_has_priority =
        fault_priv == PRIV_HS &&
        fault_cause == CAUSE_LAF &&
        fault_tval == vaddr &&
        denied_load == load_sentinel &&
        leaf_after_fault == high_ppn_leaf &&
        (leaf_after_fault & PTE_AD) == 0ULL &&
        read64(paddr) == seed &&
        read64(guard) == guard_seed &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL;

    *leaf = fixed_leaf;
    sfence_vma();
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRW(CSR_MTVEC, saved_mtvec);

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    retry_load = ld(vaddr);
    const bool retry_load_ok =
        !excpt.triggered && retry_load == seed;
    TEST_SETUP_EXCEPT();
    sd(vaddr, store_payload);
    const bool retry_store_ok = !excpt.triggered;
    TEST_SETUP_EXCEPT();
    retry_readback = ld(vaddr);
    const bool retry_readback_ok =
        !excpt.triggered && retry_readback == store_payload;

    goto_priv(PRIV_M);
    final_leaf = *leaf;
    final_mip = CSRR(CSR_MIP);
    const bool interrupt_and_repaired_retry_ok =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (final_mip & AI_IMC_MIP_STIP) == 0ULL &&
        final_leaf == fixed_leaf &&
        retry_load_ok &&
        retry_store_ok &&
        retry_readback_ok &&
        read64(paddr) == store_payload &&
        read64(guard) == guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("HS load through a high-PPN leaf reports access fault before A/D update while STIP remains pending",
        high_ppn_fault_has_priority,
        "priv=%lx cause=%lx tval=%lx vaddr=%lx denied=%lx leaf=%lx high_ppn_leaf=%lx pending=%lx target=%lx guard=%lx",
        fault_priv, fault_cause, fault_tval, vaddr,
        denied_load, leaf_after_fault, high_ppn_leaf,
        pending_after_fault, read64(paddr), read64(guard));
    TEST_ASSERT("after repairing the high-PPN PTE and handling STIP once, retry load/store use the repaired leaf generation exactly once",
        interrupt_and_repaired_retry_ok,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx final_mip=%lx leaf=%lx retry_load=%lx readback=%lx target=%lx guard=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], final_mip, final_leaf, retry_load,
        retry_readback, read64(paddr), read64(guard));

    TEST_END(ai_interrupt_memory_hs_stip_high_ppn_fault_retry_priority_corner);
}

bool ai_interrupt_memory_hs_stip_satp_root_switch_new_context_corner()
{
    TEST_START();

    const uintptr_t alias_vaddr = hs_page_base(SWITCH1) + 0x1e40U;
    const uintptr_t global_vaddr = hs_page_base(SWITCH2) + 0x1e40U;
    const uintptr_t old_paddr = phys_page_base(SWITCH1) + 0x1e40U;
    const uintptr_t new_paddr = phys_page_base(SWITCH2) + 0x1e40U;
    const uintptr_t global_paddr = phys_page_base(SCRATCHPAD) + 0x1e40U;
    const uintptr_t old_guard = old_paddr + sizeof(uint64_t);
    const uintptr_t new_guard = new_paddr + sizeof(uint64_t);
    const uintptr_t global_guard = global_paddr + sizeof(uint64_t);
    const uint64_t old_seed = 0x696d6373616f6c64ULL;
    const uint64_t new_seed = 0x696d6373616e6577ULL;
    const uint64_t global_seed = 0x696d637361676c62ULL;
    const uint64_t new_payload = 0x696d63736173746fULL;
    const uint64_t global_payload = 0x696d637361676c73ULL;
    const uint64_t old_guard_seed = 0x696d637361676f6cULL;
    const uint64_t new_guard_seed = 0x696d637361676e65ULL;
    const uint64_t global_guard_seed = 0x696d637361676775ULL;
    const uint64_t leaf_flags = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t global_leaf_flags = leaf_flags | PTE_G;
    const uint64_t satp_old =
        ai_imc_satp_for_root_asid(ai_imc_satp_root_old,
            AI_IMC_SATP_ASID_A);
    const uint64_t satp_new =
        ai_imc_satp_for_root_asid(ai_imc_satp_root_new,
            AI_IMC_SATP_ASID_B);
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t old_context_value = 0ULL;
    uint64_t satp_after_old_load = 0ULL;
    uint64_t satp_after_switch = 0ULL;
    uint64_t satp_after_interrupt = 0ULL;
    uint64_t mip_before_interrupt = 0ULL;
    uint64_t mip_after_interrupt = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t global_old_value = 0ULL;
    uint64_t global_new_value = 0ULL;
    uint64_t global_after_store = 0ULL;
    uint64_t new_context_value = 0ULL;
    uint64_t new_context_after_store = 0ULL;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_TVM);
    write64(old_paddr, old_seed);
    write64(new_paddr, new_seed);
    write64(global_paddr, global_seed);
    write64(old_guard, old_guard_seed);
    write64(new_guard, new_guard_seed);
    write64(global_guard, global_guard_seed);
    ai_imc_install_satp_root(ai_imc_satp_root_old, ai_imc_satp_l1_old,
        ai_imc_satp_l2_old, ai_imc_satp_l3_old, alias_vaddr, old_paddr,
        leaf_flags);
    ai_imc_install_satp_root(ai_imc_satp_root_new, ai_imc_satp_l1_new,
        ai_imc_satp_l2_new, ai_imc_satp_l3_new, alias_vaddr, new_paddr,
        leaf_flags);
    ai_imc_add_satp_leaf(ai_imc_satp_l2_old, ai_imc_satp_l3_old,
        global_vaddr, global_paddr, global_leaf_flags);
    ai_imc_add_satp_leaf(ai_imc_satp_l2_new, ai_imc_satp_l3_new,
        global_vaddr, global_paddr, global_leaf_flags);
    fence_iorw_iorw();
    CSRW(CSR_SATP, satp_old);
    sfence_vma();

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRW(CSR_SIE, saved_sie & ~AI_IMC_MIE_STIE);
    TEST_SETUP_EXCEPT();
    old_context_value = ai_imc_ld_probe(alias_vaddr, 0ULL);
    global_old_value = ai_imc_ld_probe(global_vaddr, 0ULL);
    satp_after_old_load = CSRR(CSR_SATP);
    const bool old_root_translation_visible =
        !excpt.triggered &&
        old_context_value == old_seed &&
        global_old_value == global_seed &&
        satp_after_old_load == satp_old &&
        ((satp_after_old_load & AI_IMC_SATP_ASID_MASK) ==
            ((AI_IMC_SATP_ASID_A << AI_IMC_SATP_ASID_SHIFT) &
                AI_IMC_SATP_ASID_MASK)) &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_seed &&
        read64(global_paddr) == global_seed &&
        read64(old_guard) == old_guard_seed &&
        read64(new_guard) == new_guard_seed &&
        read64(global_guard) == global_guard_seed;

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 1ULL);
    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_SATP, satp_new);
    satp_after_switch = CSRR(CSR_SATP);
    mip_before_interrupt = CSRR(CSR_MIP);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    satp_after_interrupt = CSRR(CSR_SATP);
    mip_after_interrupt = CSRR(CSR_MIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    sfence_vma();
    const bool interrupt_in_satp_switch_window =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (mip_before_interrupt & AI_IMC_MIP_STIP) != 0ULL &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IMC_MIP_STIP) == 0ULL &&
        satp_after_switch == satp_new &&
        satp_after_interrupt == satp_new &&
        ((satp_after_interrupt & AI_IMC_SATP_ASID_MASK) ==
            ((AI_IMC_SATP_ASID_B << AI_IMC_SATP_ASID_SHIFT) &
                AI_IMC_SATP_ASID_MASK)) &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_seed &&
        read64(global_paddr) == global_seed;

    goto_priv(PRIV_HS);
    TEST_SETUP_EXCEPT();
    global_new_value = ai_imc_ld_probe(global_vaddr, 0ULL);
    new_context_value = ai_imc_ld_probe(alias_vaddr, 0ULL);
    sd(global_vaddr, global_payload);
    global_after_store = ai_imc_ld_probe(global_vaddr, 0ULL);
    sd(alias_vaddr, new_payload);
    new_context_after_store = ai_imc_ld_probe(alias_vaddr, 0ULL);
    const bool new_root_translation_visible =
        !excpt.triggered &&
        global_new_value == global_seed &&
        global_after_store == global_payload &&
        new_context_value == new_seed &&
        new_context_after_store == new_payload;

    goto_priv(PRIV_M);
    const bool backing_images_isolated =
        CSRR(CSR_SATP) == satp_new &&
        read64(old_paddr) == old_seed &&
        read64(new_paddr) == new_payload &&
        read64(global_paddr) == global_payload &&
        read64(old_guard) == old_guard_seed &&
        read64(new_guard) == new_guard_seed &&
        read64(global_guard) == global_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("old satp root maps the shared HS VA to the old physical image before the interrupt switch window",
        old_root_translation_visible,
        "old_value=%lx global_old=%lx satp=%lx expected_satp=%lx old_pa=%lx new_pa=%lx global_pa=%lx guards=%lx,%lx,%lx",
        old_context_value, global_old_value, satp_after_old_load, satp_old,
        read64(old_paddr), read64(new_paddr), read64(global_paddr),
        read64(old_guard), read64(new_guard), read64(global_guard));
    TEST_ASSERT("pending STIP is delivered exactly once after writing the new satp root and before the post-switch memory probe",
        interrupt_in_satp_switch_window,
        "rounds=%lx trap_count=%lx cause=%lx mip_before=%lx mip0=%lx mip_after=%lx satp_switch=%lx satp_after=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        mip_before_interrupt, ai_imc_trap_mip[0], mip_after_interrupt,
        satp_after_switch, satp_after_interrupt);
    TEST_ASSERT("after sfence.vma, the same HS VA uses ASID-B non-global mapping while the global mapping remains valid across roots",
        new_root_translation_visible && backing_images_isolated,
        "new_value=%lx new_after=%lx global_new=%lx global_after=%lx old_pa=%lx new_pa=%lx global_pa=%lx guards=%lx,%lx,%lx final_satp=%lx",
        new_context_value, new_context_after_store, global_new_value,
        global_after_store, read64(old_paddr), read64(new_paddr),
        read64(global_paddr), read64(old_guard), read64(new_guard),
        read64(global_guard), CSRR(CSR_SATP));

    TEST_END(ai_interrupt_memory_hs_stip_satp_root_switch_new_context_corner);
}

bool ai_interrupt_memory_hs_stip_vector_pbmt_io_active_fault_masked_inactive_corner()
{
    TEST_START();

    const enum test_page low_page = SWITCH1;
    const enum test_page high_page = SWITCH2;
    const enum test_page old_page = VSRWX_GRWX;
    const enum test_page dump_page = SCRATCHPAD;
    const uintptr_t fault_vaddr = hs_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_fault_vaddr = hs_page_base(high_page);
    const uintptr_t old_vaddr = hs_page_base(old_page) + 0xe00U;
    const uintptr_t dump_vaddr = hs_page_base(dump_page) + 0xe40U;
    const uintptr_t post_src_vaddr = hs_page_base(dump_page) + 0xe80U;
    const uintptr_t post_dst_vaddr = hs_page_base(dump_page) + 0xec0U;
    const uintptr_t low_paddr = phys_page_base(low_page) + PAGE_SIZE - 7U;
    const uintptr_t high_paddr = phys_page_base_offset(high_page,
        AI_IMC_PBMT_IO_MEM_BASE);
    const uintptr_t old_paddr = phys_page_base(old_page) + 0xe00U;
    const uintptr_t dump_paddr = phys_page_base(dump_page) + 0xe40U;
    const uintptr_t post_src_paddr = phys_page_base(dump_page) + 0xe80U;
    const uintptr_t post_dst_paddr = phys_page_base(dump_page) + 0xec0U;
    const uint8_t split_bytes[16] = {
        0x21U, 0x22U, 0x23U, 0x24U,
        0x25U, 0x26U, 0x27U, 0x28U,
        0x29U, 0x2aU, 0x2bU, 0x2cU,
        0x2dU, 0x2eU, 0x2fU, 0x30U
    };
    const uint32_t old_words[2] = {
        0x74737271U, 0x78777675U
    };
    const uint32_t post_words[AI_IMC_VEC_VL4] = {
        0xa1480000U, 0xa1480001U, 0xa1480002U, 0xa1480003U
    };
    const uint32_t expected_masked[2] = {
        0x24232221U, old_words[1]
    };
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t active_fault_vstart = 0xffffffffffffffffULL;
    uint64_t active_fault_cause = 0ULL;
    uint64_t active_fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t trap_count_after_fault = 0ULL;
    uint64_t fault_cause_after_interrupt = 0ULL;
    uint64_t fault_tval_after_interrupt = 0ULL;
    uint64_t vstart_after_interrupt = 0xffffffffffffffffULL;
    uint64_t masked_vstart = 0xffffffffffffffffULL;
    uint64_t post_vstart = 0xffffffffffffffffULL;
    uint64_t interrupt_rounds = 0ULL;
    uint32_t low_word_after = 0U;
    uint32_t high_word_after = 0U;
    uint32_t dump_word0_after = 0U;
    uint32_t dump_word1_after = 0U;
    uint32_t post_word0_after = 0U;
    uint32_t post_word1_after = 0U;
    uint32_t post_word2_after = 0U;
    uint32_t post_word3_after = 0U;
    uint64_t mip_after_interrupt = 0ULL;

    goto_priv(PRIV_M);
    ai_imc_prepare_pbmt_io_mem_window();
    ai_imc_write_bytes(low_paddr, split_bytes, 7ULL);
    ai_imc_write_bytes(high_paddr, split_bytes + 7U, 9ULL);
    for (uint64_t i = 0ULL; i < 2ULL; ++i) {
        write32(old_paddr + i * sizeof(uint32_t), old_words[i]);
        write32(dump_paddr + i * sizeof(uint32_t), 0xeeee0000U + i);
    }
    for (uint64_t i = 0ULL; i < AI_IMC_VEC_VL4; ++i) {
        write32(post_src_paddr + i * sizeof(uint32_t), post_words[i]);
        write32(post_dst_paddr + i * sizeof(uint32_t), 0xdddd0000U + i);
    }
    fence_iorw_iorw();

    ai_imc_enable_vector_state();
    ai_imc_reset_trap_log();
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(low_page, low_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    ai_imc_write_hs_leaf_to_paddr(high_page, high_paddr,
        PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt2, 0ULL);
    ai_imc_write_hs_leaf_to_page(old_page, old_page,
        PTE_V | PTE_AD | PTE_RWX, 0ULL);
    ai_imc_write_hs_leaf_to_page(dump_page, dump_page,
        PTE_V | PTE_AD | PTE_RWX, 1ULL);

    goto_priv(PRIV_M);
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);

    goto_priv(PRIV_HS);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)fault_vaddr, AI_IMC_VEC_VL4);
    active_fault_vstart = CSRR(CSR_VSTART);
    active_fault_cause = excpt.cause;
    active_fault_tval = excpt.tval;

    goto_priv(PRIV_M);
    pending_after_fault = CSRR(CSR_MIP);
    trap_count_after_fault = ai_imc_trap_count;
    const bool active_pbmt_io_lane_fault =
        excpt.triggered &&
        active_fault_cause == CAUSE_LAF &&
        active_fault_tval == high_fault_vaddr &&
        active_fault_vstart == 1ULL &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        trap_count_after_fault == 0ULL &&
        ai_imc_bytes_match(low_paddr, split_bytes, 7ULL) &&
        ai_imc_bytes_match(high_paddr, split_bytes + 7U, 9ULL);

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE | AI_IMC_MSTATUS_VS_DIRTY);
    interrupt_rounds = ai_imc_spin_until_log_count(1ULL,
        AI_IMC_SPIN_LIMIT);
    fault_cause_after_interrupt = excpt.cause;
    fault_tval_after_interrupt = excpt.tval;
    vstart_after_interrupt = CSRR(CSR_VSTART);
    mip_after_interrupt = CSRR(CSR_MIP);
    CSRW(CSR_MTVEC, saved_mtvec);
    const bool interrupt_delivered_once =
        interrupt_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        (mip_after_interrupt & AI_IMC_MIP_STIP) == 0ULL &&
        vstart_after_interrupt == active_fault_vstart &&
        fault_cause_after_interrupt == active_fault_cause &&
        fault_tval_after_interrupt == active_fault_tval;

    goto_priv(PRIV_HS);
    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    ai_imc_vec_masked_vle32_lane0_dump((const uint32_t *)old_vaddr,
        (const uint32_t *)fault_vaddr, (uint32_t *)dump_vaddr,
        &masked_vstart);
    const bool masked_inactive_pbmt_io_suppressed =
        !excpt.triggered &&
        masked_vstart == 0ULL;

    CSRW(CSR_VSTART, 0ULL);
    TEST_SETUP_EXCEPT();
    vle32_to_v6((const uint32_t *)post_src_vaddr, AI_IMC_VEC_VL4);
    vse32_from_v6((uint32_t *)post_dst_vaddr, AI_IMC_VEC_VL4);
    post_vstart = CSRR(CSR_VSTART);
    const bool post_dram_vector_ok =
        !excpt.triggered &&
        post_vstart == 0ULL;

    goto_priv(PRIV_M);
    low_word_after = read32(low_paddr);
    high_word_after = read32(high_paddr);
    dump_word0_after = read32(dump_paddr);
    dump_word1_after = read32(dump_paddr + sizeof(uint32_t));
    post_word0_after = read32(post_dst_paddr);
    post_word1_after = read32(post_dst_paddr + sizeof(uint32_t));
    post_word2_after = read32(post_dst_paddr + 2U * sizeof(uint32_t));
    post_word3_after = read32(post_dst_paddr + 3U * sizeof(uint32_t));
    const bool masked_dump_and_post_image_ok =
        masked_inactive_pbmt_io_suppressed &&
        dump_word0_after == expected_masked[0] &&
        dump_word1_after == expected_masked[1] &&
        post_dram_vector_ok &&
        ai_imc_words_match(post_dst_paddr, post_words, AI_IMC_VEC_VL4) &&
        ai_imc_bytes_match(low_paddr, split_bytes, 7ULL) &&
        ai_imc_bytes_match(high_paddr, split_bytes + 7U, 9ULL);

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    CSRW(CSR_VSTART, 0ULL);
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    ai_imc_reset_pbmt_io_mem_window();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("active vector lane that crosses into PBMT=IO reports load access fault with high-page tval while pending STIP is not taken",
        active_pbmt_io_lane_fault,
        "cause=%lx tval=%lx expected_tval=%lx vstart=%lx mip=%lx trap_count=%lx low0=%x high0=%x",
        active_fault_cause, active_fault_tval, high_fault_vaddr,
        active_fault_vstart, pending_after_fault, trap_count_after_fault,
        low_word_after, high_word_after);
    TEST_ASSERT("the preserved STIP after the PBMT=IO vector fault is delivered once and clears pending state",
        interrupt_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx mip0=%lx final_mip=%lx vstart_after_irq=%lx expected_vstart=%lx fault_after=%lx/%lx",
        interrupt_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], mip_after_interrupt, vstart_after_interrupt,
        active_fault_vstart, fault_cause_after_interrupt,
        fault_tval_after_interrupt);
    TEST_ASSERT("masked inactive lane pointing at the PBMT=IO split does not ghost fault or overwrite vstart/tval, and ordinary DRAM vector traffic still works",
        masked_dump_and_post_image_ok,
        "masked_vstart=%lx dump=%x,%x expected=%x,%x post_vstart=%lx post=%x,%x,%x,%x",
        masked_vstart, dump_word0_after, dump_word1_after,
        expected_masked[0], expected_masked[1], post_vstart,
        post_word0_after, post_word1_after, post_word2_after,
        post_word3_after);

    TEST_END(ai_interrupt_memory_hs_stip_vector_pbmt_io_active_fault_masked_inactive_corner);
}

bool ai_interrupt_memory_m_stip_trace_commit_event_order_audit_corner()
{
    TEST_START();

    const enum test_page fault_page = SWITCH1;
    const uintptr_t fault_paddr = phys_page_base(fault_page) + 0xf00U;
    const uintptr_t fault_vaddr = hs_page_base(fault_page) + 0xf00U;
    const uintptr_t store_addr = phys_page_base(VSRWX_GRWX) + 0xf40U;
    const uintptr_t load_addr = phys_page_base(VSRWX_GRWX) + 0xf48U;
    const uintptr_t amo_addr = phys_page_base(VSRWX_GRWX) + 0xf50U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0xf58U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0xf60U;
    const uintptr_t pmp_start = fault_paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t fault_seed = 0x696d637472616630ULL;
    const uint64_t fault_attempt = 0x696d6374726166ddULL;
    const uint64_t store_value = 0x696d637472737431ULL;
    const uint64_t load_seed = 0x696d6374726c6432ULL;
    const uint64_t amo_seed = 0x696d637472616d33ULL;
    const uint64_t amo_addend = 0x31ULL;
    const uint64_t marker_value = 0x696d6374726d6b34ULL;
    const uint64_t guard_seed = 0x696d637472677535ULL;
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
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_after_fault = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t load_value = 0ULL;
    uint64_t amo_old = 0ULL;

    goto_priv(PRIV_M);
    write64(fault_paddr, fault_seed);
    write64(store_addr, 0ULL);
    write64(load_addr, load_seed);
    write64(amo_addr, amo_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imc_restore_pmp_full_access();
    ai_imc_prepare_mprv_user_translation();
    ai_imc_reset_trap_log();
    ai_imc_prepare_stip(saved_mideleg, 0ULL);
    CSRS(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_configure_pmp_window(pmp_start, pmp_end,
        AI_IMC_PMP_TOR_NO_W);

    ai_imc_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    sd(fault_vaddr, fault_attempt);
    ai_imc_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    pending_after_fault = CSRR(CSR_MIP);

    const bool store_fault_event_is_precise =
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        fault_cause == CAUSE_SAF &&
        fault_tval == fault_vaddr &&
        (pending_after_fault & AI_IMC_MIP_STIP) != 0ULL &&
        read64(fault_paddr) == fault_seed;

    ai_imc_reset_trap_log();
    CSRW(CSR_MTVEC, (uintptr_t)ai_imc_mtvec_log_handler);
    CSRS(CSR_MIE, AI_IMC_MIE_STIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MTVEC, saved_mtvec);

    write64(store_addr, store_value);
    load_value = read64(load_addr);
    amo_old = ai_imc_amoadd_d_aqrl((uint64_t *)amo_addr, amo_addend);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool interrupt_event_between_fault_and_memory_once =
        rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_STI &&
        (ai_imc_trap_mip[0] & AI_IMC_MIP_STIP) != 0ULL &&
        ai_imc_handler_log_page[0] == 0x696d63686c6f6700ULL &&
        (CSRR(CSR_MIP) & AI_IMC_MIP_STIP) == 0ULL;
    const bool memory_events_have_distinct_owners =
        read64(fault_paddr) == fault_seed &&
        read64(store_addr) == store_value &&
        load_value == load_seed &&
        read64(load_addr) == load_seed &&
        amo_old == amo_seed &&
        read64(amo_addr) == amo_seed + amo_addend &&
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed;

    goto_priv(PRIV_M);
    ai_imc_clear_mprv_translation();
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_MIP_STIP);
    ai_imc_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("store access fault is reported with its own PC/tval owner while the pending STIP remains undelivered for trace-order audit",
        store_fault_event_is_precise,
        "cause=%lx tval=%lx expected=%lx pending=%lx fault_mem=%lx",
        fault_cause, fault_tval, fault_vaddr, pending_after_fault,
        read64(fault_paddr));
    TEST_ASSERT("the preserved STIP is delivered exactly once between the fault episode and the later memory-event sequence",
        interrupt_event_between_fault_and_memory_once,
        "rounds=%lx trap_count=%lx cause=%lx mip=%lx log=%lx final_mip=%lx",
        rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_log_page[0],
        CSRR(CSR_MIP));
    TEST_ASSERT("post-interrupt store, load, AMO and marker have distinct payloads for LinkNan commit/MemTrace/L1MissTrace owner comparison",
        memory_events_have_distinct_owners,
        "store=%lx load=%lx amo_old=%lx amo_final=%lx marker=%lx guard=%lx",
        read64(store_addr), load_value, amo_old, read64(amo_addr),
        read64(marker_addr), read64(guard_addr));

    TEST_END(ai_interrupt_memory_m_stip_trace_commit_event_order_audit_corner);
}

bool ai_interrupt_memory_hs_stip_pbmt_nc_fetch_data_owner_audit_corner()
{
    TEST_START();

    typedef uint64_t (*ai_imc_fetch_func_t)(void);

    const enum test_page code_page = VSRWX_GRWX;
    const enum test_page data_page = SWITCH1;
    const enum test_page marker_page = SCRATCHPAD;
    const uintptr_t offset = 0xf80U;
    const uintptr_t code_vaddr = hs_page_base(code_page) + offset;
    const uintptr_t code_paddr = phys_page_base(code_page) + offset;
    const uintptr_t data_vaddr = hs_page_base(data_page) + offset;
    const uintptr_t data_paddr = phys_page_base(data_page) + offset;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + offset;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + offset;
    const uintptr_t code_guard = code_paddr + sizeof(uint64_t);
    const uintptr_t data_guard = data_paddr + sizeof(uint64_t);
    const uintptr_t marker_guard = marker_paddr + sizeof(uint64_t);
    const uint64_t old_code = 0x0000806702100513ULL;
    const uint64_t new_code = 0x0000806704300513ULL;
    const uint64_t old_return = 0x21ULL;
    const uint64_t new_return = 0x43ULL;
    const uint64_t data_payload = 0x696d636e63646174ULL;
    const uint64_t marker_value = 0x696d636e636d6b21ULL;
    const uint64_t code_guard_seed = 0x696d636e63636731ULL;
    const uint64_t data_guard_seed = 0x696d636e63646732ULL;
    const uint64_t marker_guard_seed = 0x696d636e63676733ULL;
    const uint64_t nc_leaf = PTE_V | PTE_AD | PTE_RWX | PTE_Pbmt1;
    const uint64_t cacheable_leaf = PTE_V | PTE_AD | PTE_RWX;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_sepc = CSRR(CSR_SEPC);
    ai_imc_fetch_func_t fetch_func = (ai_imc_fetch_func_t)code_vaddr;
    uint64_t old_value = 0ULL;
    uint64_t old_fetch_triggered = 1ULL;
    uint64_t handler_rounds = 0ULL;
    uint64_t main_data_after_handler = 0ULL;
    uint64_t patch_triggered = 1ULL;
    uint64_t final_value = 0ULL;
    uint64_t final_fetch_triggered = 1ULL;

    goto_priv(PRIV_M);
    CSRS(CSR_MENVCFG, MENVCFG_PBMTE);
    write64(code_paddr, old_code);
    write64(data_paddr, data_payload);
    write64(marker_paddr, 0ULL);
    write64(code_guard, code_guard_seed);
    write64(data_guard, data_guard_seed);
    write64(marker_guard, marker_guard_seed);
    fence_iorw_iorw();

    goto_priv(PRIV_HS);
    hspt_init();
    ai_imc_write_hs_leaf_to_page(code_page, code_page, nc_leaf, 1ULL);
    ai_imc_write_hs_leaf_to_page(data_page, data_page, nc_leaf, 1ULL);
    ai_imc_write_hs_leaf_to_page(marker_page, marker_page,
        cacheable_leaf, 1ULL);
    CSRC(CSR_SSTATUS, SSTATUS_MXR | SSTATUS_SUM | SSTATUS_SIE_BIT);
    sfence_vma();
    asm volatile("fence.i" ::: "memory");

    TEST_SETUP_EXCEPT();
    old_value = fetch_func();
    old_fetch_triggered = excpt.triggered;
    const bool old_nc_fetch_uses_old_instruction_image =
        !old_fetch_triggered &&
        old_value == old_return &&
        read64(code_paddr) == old_code &&
        read64(data_paddr) == data_payload &&
        read64(code_guard) == code_guard_seed &&
        read64(data_guard) == data_guard_seed;

    ai_imc_reset_trap_log();
    ai_imc_handler_probe_addr = data_vaddr;
    CSRW(CSR_STVEC, (uintptr_t)ai_imc_stvec_same_target_load_handler);
    ai_imc_prepare_hs_ssip(1ULL);
    goto_priv(PRIV_HS);
    CSRS(CSR_SIP, AI_IMC_SIP_SSIP);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
    handler_rounds = ai_imc_spin_until_log_count(1ULL, AI_IMC_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    main_data_after_handler = ai_imc_ld_probe(data_vaddr, 0ULL);

    TEST_SETUP_EXCEPT();
    sd(code_vaddr, new_code);
    fence_iorw_iorw();
    asm volatile("fence.i" ::: "memory");
    patch_triggered = excpt.triggered;
    final_value = fetch_func();
    final_fetch_triggered = excpt.triggered;
    sd(marker_vaddr, marker_value);
    fence_iorw_iorw();

    const bool handler_nc_data_load_is_single_owner =
        handler_rounds < AI_IMC_SPIN_LIMIT &&
        ai_imc_trap_count == 1ULL &&
        ai_imc_trap_cause[0] == CAUSE_SSI &&
        (ai_imc_trap_mip[0] & AI_IMC_SIP_SSIP) != 0ULL &&
        ai_imc_handler_probe_value[0] == data_payload &&
        main_data_after_handler == data_payload &&
        ai_imc_handler_log_page[0] == 0x696d6373686c6700ULL &&
        (CSRR(CSR_SIP) & AI_IMC_SIP_SSIP) == 0ULL;
    const bool final_nc_fetch_uses_patched_instruction_owner =
        !patch_triggered &&
        !final_fetch_triggered &&
        final_value == new_return &&
        final_value != old_return &&
        read64(code_paddr) == new_code &&
        read64(data_paddr) == data_payload &&
        read64(marker_paddr) == marker_value &&
        read64(code_guard) == code_guard_seed &&
        read64(data_guard) == data_guard_seed &&
        read64(marker_guard) == marker_guard_seed;

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_MIP, AI_IMC_SIP_SSIP);
    ai_imc_handler_probe_addr = 0ULL;
    goto_priv(PRIV_HS);
    CSRW(CSR_SSTATUS, saved_sstatus);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_SEPC, saved_sepc);
    goto_priv(PRIV_M);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MENVCFG, saved_menvcfg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MSTATUS, saved_mstatus);

    TEST_ASSERT("PBMT=NC executable fetch returns the old code image before the interrupt/data-load audit window",
        old_nc_fetch_uses_old_instruction_image,
        "old_value=%lx expected=%lx triggered=%lx code=%lx data=%lx guards=%lx,%lx",
        old_value, old_return, old_fetch_triggered, read64(code_paddr),
        read64(data_paddr), read64(code_guard), read64(data_guard));
    TEST_ASSERT("delegated SSIP handler performs exactly one PBMT=NC data-side load and clears only its interrupt source",
        handler_nc_data_load_is_single_owner,
        "rounds=%lx trap_count=%lx cause=%lx sip=%lx probe=%lx main_data=%lx log=%lx final_sip=%lx",
        handler_rounds, ai_imc_trap_count, ai_imc_trap_cause[0],
        ai_imc_trap_mip[0], ai_imc_handler_probe_value[0],
        main_data_after_handler, ai_imc_handler_log_page[0],
        CSRR(CSR_SIP));
    TEST_ASSERT("after D-side patch and fence.i, PBMT=NC fetch executes the new code image without importing handler data payload",
        final_nc_fetch_uses_patched_instruction_owner,
        "patch_trig=%lx final_trig=%lx final=%lx old=%lx new=%lx code=%lx data=%lx marker=%lx guards=%lx,%lx,%lx",
        patch_triggered, final_fetch_triggered, final_value, old_return,
        new_return, read64(code_paddr), read64(data_paddr),
        read64(marker_paddr), read64(code_guard), read64(data_guard),
        read64(marker_guard));

    TEST_END(ai_interrupt_memory_hs_stip_pbmt_nc_fetch_data_owner_audit_corner);
}
