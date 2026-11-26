#include <rvh_test.h>
#include <page_tables.h>
#include <stdint.h>

/*
 * VModule-only interrupt/memory cross cases.
 *
 * These cases intentionally do not use per-case #if VMODULE branches. Runner
 * selection owns the boundary: current ordinary LinkNan/Spike must not
 * register or run these cases. A valid run needs a VModule-capable
 * AP-IT/special-run/RTL environment with a whole-ELF VMODULE=1 build.
 * VMODULE=1 also changes the common interrupt handler source-clear path in
 * src/rvh_test.c, so VModule builds are not interchangeable with ordinary
 * CSR-pending interrupt gates.
 */

#define AI_IMV_VMODULE_REG_MAIN_INTERRUPT 0x1000ULL
#define AI_IMV_VMODULE_REG_INTERRUPT_DELAY 0x1028ULL
#define AI_IMV_VMODULE_REG_FORCE_NMI 0x1030ULL
#define AI_IMV_VMODULE_REG_CHI_ERROR 0x1050ULL
#define AI_IMV_VMODULE_REG_AXI_ERROR 0x1060ULL
#define AI_IMV_VMODULE_MMIO_BASE 0x40600000ULL
#define AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT \
    (AI_IMV_VMODULE_MMIO_BASE + AI_IMV_VMODULE_REG_MAIN_INTERRUPT)
#define AI_IMV_VMODULE_MAIN_SEI 0x00000200ULL
#define AI_IMV_VMODULE_MAIN_MEI 0x00000800ULL
#define AI_IMV_NMI31_CAUSE (CAUSE_INT_BIT | 31ULL)
#define AI_IMV_SPIN_LIMIT 1024ULL
#define AI_IMV_LOG_SLOTS 4U
#define AI_IMV_MSTATUS_MPP_MASK (3ULL << 11)
#define AI_IMV_PMP_TOR_RWX 0x0fULL
#define AI_IMV_PMP_TOR_NO_R 0x0cULL

extern void ai_imv_mtvec_descriptor_ack_handler(void);
extern void ai_imv_mtvec_error_rnmi_mei_handler(void);
extern void ai_imv_mtvec_mei_clear_window_rnmi_handler(void);
extern void ai_imv_stvec_sei_clear_store_fault_retry_handler(void);
extern void mhandler_entry(void);

volatile uint64_t ai_imv_trap_count;
volatile uint64_t ai_imv_trap_cause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_trap_mip[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_desc_seq_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_desc_payload_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_desc_checksum_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_desc_tail_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_desc_ack_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_source_clear_log[AI_IMV_LOG_SLOTS];
volatile uintptr_t ai_imv_desc_ring_base_addr;
volatile uintptr_t ai_imv_desc_tail_addr;
volatile uintptr_t ai_imv_desc_ack_base_addr;
volatile uint64_t ai_imv_error_rnmi_count;
volatile uint64_t ai_imv_error_rnmi_mncause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_rnmi_mnepc[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_rnmi_mnstatus[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_rnmi_mip[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_rnmi_old_value[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_rnmi_marker_value[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_mnret_markers;
volatile uint64_t ai_imv_error_ordinary_count;
volatile uint64_t ai_imv_error_ordinary_cause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_ordinary_mip[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_error_ordinary_marker_value[AI_IMV_LOG_SLOTS];
volatile uintptr_t ai_imv_error_old_addr;
volatile uintptr_t ai_imv_error_marker_addr;
volatile uint64_t ai_imv_window_ordinary_count;
volatile uint64_t ai_imv_window_ordinary_cause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_mip_before[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_mip_after_rnmi[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_mip_after_clear[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_marker_before[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_marker_after_rnmi[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_ordinary_clear_log[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_rnmi_count;
volatile uint64_t ai_imv_window_rnmi_mncause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_rnmi_mnstatus[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_rnmi_mip[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_rnmi_marker_value[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_window_rnmi_mnret_markers;
volatile uintptr_t ai_imv_window_marker_addr;
volatile uintptr_t ai_imv_window_ordinary_log_addr;
volatile uintptr_t ai_imv_window_rnmi_log_addr;
volatile uint64_t ai_imv_clear_retry_interrupt_count;
volatile uint64_t ai_imv_clear_retry_fault_count;
volatile uint64_t ai_imv_clear_retry_clear_success_count;
volatile uint64_t ai_imv_clear_retry_scause[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_stval[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_sip_before_clear[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_sip_on_fault[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_sip_after_clear[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_marker_before[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_marker_after[AI_IMV_LOG_SLOTS];
volatile uint64_t ai_imv_clear_retry_faults_seen_at_clear;
volatile uintptr_t ai_imv_clear_retry_marker_addr;
volatile uintptr_t ai_imv_clear_retry_log_addr;
volatile uintptr_t ai_imv_clear_retry_store_pte_addr;
volatile uint64_t ai_imv_clear_retry_store_pte_value;

static pte_t ai_imv_vmodule_mmio_l2[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));
static pte_t ai_imv_vmodule_mmio_l3[PAGE_SIZE / sizeof(pte_t)]
    __attribute__((aligned(PAGE_SIZE)));

asm(
    ".align 2\n"
    ".global ai_imv_mtvec_descriptor_ack_handler\n"
    "ai_imv_mtvec_descriptor_ack_handler:\n"
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
    "  csrr t1, mip\n"
    "  la t2, ai_imv_trap_count\n"
    "  ld t3, 0(t2)\n"
    "  li t4, 4\n"
    "  bgeu t3, t4, .Lai_imv_desc_clear_source\n"
    "  slli t5, t3, 3\n"
    "  la t6, ai_imv_trap_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_trap_mip\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_desc_ring_base_addr\n"
    "  ld a0, 0(t6)\n"
    "  la t6, ai_imv_desc_tail_addr\n"
    "  ld a1, 0(t6)\n"
    "  beqz a0, .Lai_imv_desc_log_done\n"
    "  beqz a1, .Lai_imv_desc_log_done\n"
    "  ld a2, 0(a1)\n"
    "  slli a3, a2, 5\n"
    "  add a0, a0, a3\n"
    "  ld t0, 0(a0)\n"
    "  ld t1, 8(a0)\n"
    "  ld t4, 16(a0)\n"
    "  la t6, ai_imv_desc_seq_log\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_desc_payload_log\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_desc_checksum_log\n"
    "  add t6, t6, t5\n"
    "  sd t4, 0(t6)\n"
    "  la t6, ai_imv_desc_tail_log\n"
    "  add t6, t6, t5\n"
    "  sd a2, 0(t6)\n"
    "  la t6, ai_imv_desc_ack_base_addr\n"
    "  ld t6, 0(t6)\n"
    "  beqz t6, .Lai_imv_desc_advance_tail\n"
    "  slli a3, a2, 3\n"
    "  add t6, t6, a3\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_desc_ack_log\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    ".Lai_imv_desc_advance_tail:\n"
    "  addi a2, a2, 1\n"
    "  sd a2, 0(a1)\n"
    ".Lai_imv_desc_log_done:\n"
    "  la t6, ai_imv_source_clear_log\n"
    "  add t6, t6, t5\n"
    "  li t0, 1\n"
    "  sd t0, 0(t6)\n"
    "  addi t3, t3, 1\n"
    "  sd t3, 0(t2)\n"
    ".Lai_imv_desc_clear_source:\n"
    "  li t0, 0xffff000100001000\n"
    "  li t1, 0xffff000000000000\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  li t0, 0x800\n"
    "  csrc mip, t0\n"
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
    ".global ai_imv_mtvec_error_rnmi_mei_handler\n"
    "ai_imv_mtvec_error_rnmi_mei_handler:\n"
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
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_imv_error_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_imv_error_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  beq t2, t3, .Lai_imv_error_rnmi\n"
    "  li t3, 43\n"
    "  bne t2, t3, .Lai_imv_error_ordinary\n"
    ".Lai_imv_error_rnmi:\n"
    "  csrr t2, 0x741\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imv_error_rnmi_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imv_error_clear_error_sources\n"
    "  slli a0, t5, 3\n"
    "  la t6, ai_imv_error_rnmi_mncause\n"
    "  add t6, t6, a0\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_error_rnmi_mnepc\n"
    "  add t6, t6, a0\n"
    "  sd t2, 0(t6)\n"
    "  la t6, ai_imv_error_rnmi_mnstatus\n"
    "  add t6, t6, a0\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_error_rnmi_mip\n"
    "  add t6, t6, a0\n"
    "  sd t3, 0(t6)\n"
    "  la t6, ai_imv_error_old_addr\n"
    "  ld a1, 0(t6)\n"
    "  li a2, 0\n"
    "  beqz a1, .Lai_imv_error_old_logged\n"
    "  ld a2, 0(a1)\n"
    ".Lai_imv_error_old_logged:\n"
    "  la t6, ai_imv_error_rnmi_old_value\n"
    "  add t6, t6, a0\n"
    "  sd a2, 0(t6)\n"
    "  la t6, ai_imv_error_marker_addr\n"
    "  ld a1, 0(t6)\n"
    "  li a2, 0\n"
    "  beqz a1, .Lai_imv_error_marker_logged\n"
    "  ld a2, 0(a1)\n"
    ".Lai_imv_error_marker_logged:\n"
    "  la t6, ai_imv_error_rnmi_marker_value\n"
    "  add t6, t6, a0\n"
    "  sd a2, 0(t6)\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imv_error_clear_error_sources:\n"
    "  li t0, 0xffff000100001050\n"
    "  li t1, 0xffff000000000000\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  li t0, 0xffff000100001060\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  li t0, 0xffff000100001030\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  la t5, ai_imv_error_mnret_markers\n"
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
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_imv_error_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mip\n"
    "  la t2, ai_imv_error_ordinary_count\n"
    "  ld t3, 0(t2)\n"
    "  li t4, 4\n"
    "  bgeu t3, t4, .Lai_imv_error_clear_mei_source\n"
    "  slli t5, t3, 3\n"
    "  la t6, ai_imv_error_ordinary_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_error_ordinary_mip\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_error_marker_addr\n"
    "  ld a0, 0(t6)\n"
    "  li a1, 0\n"
    "  beqz a0, .Lai_imv_error_ordinary_marker_logged\n"
    "  ld a1, 0(a0)\n"
    ".Lai_imv_error_ordinary_marker_logged:\n"
    "  la t6, ai_imv_error_ordinary_marker_value\n"
    "  add t6, t6, t5\n"
    "  sd a1, 0(t6)\n"
    "  addi t3, t3, 1\n"
    "  sd t3, 0(t2)\n"
    ".Lai_imv_error_clear_mei_source:\n"
    "  li t0, 0xffff000100001000\n"
    "  li t1, 0xffff000000000000\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  li t0, 0x800\n"
    "  csrc mip, t0\n"
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
    ".global ai_imv_mtvec_mei_clear_window_rnmi_handler\n"
    "ai_imv_mtvec_mei_clear_window_rnmi_handler:\n"
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
    "  csrr t0, 0x744\n"
    "  li t1, 8\n"
    "  and t2, t0, t1\n"
    "  bnez t2, .Lai_imv_window_ordinary\n"
    "  csrr t1, 0x742\n"
    "  srli t2, t1, 63\n"
    "  beqz t2, .Lai_imv_window_ordinary\n"
    "  slli t2, t1, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 31\n"
    "  bne t2, t3, .Lai_imv_window_ordinary\n"
    "  csrr t3, mip\n"
    "  la t4, ai_imv_window_rnmi_count\n"
    "  ld t5, 0(t4)\n"
    "  li t6, 4\n"
    "  bgeu t5, t6, .Lai_imv_window_clear_nmi\n"
    "  slli a0, t5, 3\n"
    "  la t6, ai_imv_window_rnmi_mncause\n"
    "  add t6, t6, a0\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_window_rnmi_mnstatus\n"
    "  add t6, t6, a0\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_window_rnmi_mip\n"
    "  add t6, t6, a0\n"
    "  sd t3, 0(t6)\n"
    "  la t6, ai_imv_window_marker_addr\n"
    "  ld a1, 0(t6)\n"
    "  li a2, 0\n"
    "  beqz a1, .Lai_imv_window_rnmi_marker_logged\n"
    "  ld a2, 0(a1)\n"
    ".Lai_imv_window_rnmi_marker_logged:\n"
    "  la t6, ai_imv_window_rnmi_marker_value\n"
    "  add t6, t6, a0\n"
    "  sd a2, 0(t6)\n"
    "  la t6, ai_imv_window_rnmi_log_addr\n"
    "  ld a1, 0(t6)\n"
    "  beqz a1, .Lai_imv_window_rnmi_log_done\n"
    "  li a2, 0x696d76776e6d6931\n"
    "  add a2, a2, t5\n"
    "  sd a2, 0(a1)\n"
    ".Lai_imv_window_rnmi_log_done:\n"
    "  addi t5, t5, 1\n"
    "  sd t5, 0(t4)\n"
    ".Lai_imv_window_clear_nmi:\n"
    "  li t0, 0xffff000100001030\n"
    "  li t1, 0xffff000000000000\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  la t5, ai_imv_window_rnmi_mnret_markers\n"
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
    "  ld a0, 56(sp)\n"
    "  ld a1, 64(sp)\n"
    "  ld a2, 72(sp)\n"
    "  ld a3, 80(sp)\n"
    "  addi sp, sp, 96\n"
    "  .insn r 0x73, 0, 0x38, x0, x0, x2\n"
    ".Lai_imv_window_ordinary:\n"
    "  csrr t0, mcause\n"
    "  csrr t1, mip\n"
    "  la t2, ai_imv_window_ordinary_count\n"
    "  ld t3, 0(t2)\n"
    "  li t4, 4\n"
    "  bgeu t3, t4, .Lai_imv_window_ordinary_clear_mei\n"
    "  slli t5, t3, 3\n"
    "  la t6, ai_imv_window_ordinary_cause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_window_ordinary_mip_before\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  la t6, ai_imv_window_marker_addr\n"
    "  ld a0, 0(t6)\n"
    "  li a1, 0\n"
    "  beqz a0, .Lai_imv_window_ordinary_marker_before_logged\n"
    "  ld a1, 0(a0)\n"
    ".Lai_imv_window_ordinary_marker_before_logged:\n"
    "  la t6, ai_imv_window_ordinary_marker_before\n"
    "  add t6, t6, t5\n"
    "  sd a1, 0(t6)\n"
    "  li t6, 0xffff000100001030\n"
    "  li a2, 0xffff000000000001\n"
    "  fence.i\n"
    "  add x0, x0, t6\n"
    "  fence.i\n"
    "  add x0, x0, a2\n"
    "  fence.i\n"
    "  li a2, 256\n"
    ".Lai_imv_window_wait_rnmi:\n"
    "  la t6, ai_imv_window_rnmi_count\n"
    "  ld a3, 0(t6)\n"
    "  bne a3, t3, .Lai_imv_window_after_rnmi\n"
    "  addi a2, a2, -1\n"
    "  bnez a2, .Lai_imv_window_wait_rnmi\n"
    ".Lai_imv_window_after_rnmi:\n"
    "  csrr a2, mip\n"
    "  la t6, ai_imv_window_ordinary_mip_after_rnmi\n"
    "  add t6, t6, t5\n"
    "  sd a2, 0(t6)\n"
    "  la t6, ai_imv_window_marker_addr\n"
    "  ld a0, 0(t6)\n"
    "  li a1, 0\n"
    "  beqz a0, .Lai_imv_window_ordinary_marker_after_logged\n"
    "  ld a1, 0(a0)\n"
    ".Lai_imv_window_ordinary_marker_after_logged:\n"
    "  la t6, ai_imv_window_ordinary_marker_after_rnmi\n"
    "  add t6, t6, t5\n"
    "  sd a1, 0(t6)\n"
    "  la t6, ai_imv_window_ordinary_log_addr\n"
    "  ld a0, 0(t6)\n"
    "  beqz a0, .Lai_imv_window_ordinary_log_done\n"
    "  li a1, 0x696d76776d656930\n"
    "  add a1, a1, t3\n"
    "  sd a1, 0(a0)\n"
    ".Lai_imv_window_ordinary_log_done:\n"
    "  la t6, ai_imv_window_ordinary_clear_log\n"
    "  add t6, t6, t5\n"
    "  li a1, 1\n"
    "  sd a1, 0(t6)\n"
    "  addi t3, t3, 1\n"
    "  sd t3, 0(t2)\n"
    ".Lai_imv_window_ordinary_clear_mei:\n"
    "  li t0, 0xffff000100001000\n"
    "  li t1, 0xffff000000000000\n"
    "  fence.i\n"
    "  add x0, x0, t0\n"
    "  fence.i\n"
    "  add x0, x0, t1\n"
    "  fence.i\n"
    "  li t0, 0x800\n"
    "  csrc mip, t0\n"
    "  csrr t1, mip\n"
    "  la t2, ai_imv_window_ordinary_count\n"
    "  ld t3, 0(t2)\n"
    "  beqz t3, .Lai_imv_window_restore_ordinary\n"
    "  addi t3, t3, -1\n"
    "  li t4, 4\n"
    "  bgeu t3, t4, .Lai_imv_window_restore_ordinary\n"
    "  slli t5, t3, 3\n"
    "  la t6, ai_imv_window_ordinary_mip_after_clear\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    ".Lai_imv_window_restore_ordinary:\n"
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
    ".global ai_imv_stvec_sei_clear_store_fault_retry_handler\n"
    "ai_imv_stvec_sei_clear_store_fault_retry_handler:\n"
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
    "  csrr t0, scause\n"
    "  csrr t1, stval\n"
    "  srli t2, t0, 63\n"
    "  beqz t2, .Lai_imv_clear_retry_sync_fault\n"
    "  slli t2, t0, 1\n"
    "  srli t2, t2, 1\n"
    "  li t3, 9\n"
    "  bne t2, t3, .Lai_imv_clear_retry_restore\n"
    "  la t2, ai_imv_clear_retry_interrupt_count\n"
    "  ld t3, 0(t2)\n"
    "  li t4, 4\n"
    "  bgeu t3, t4, .Lai_imv_clear_retry_clear_source\n"
    "  slli t5, t3, 3\n"
    "  csrr t6, sip\n"
    "  la a0, ai_imv_clear_retry_scause\n"
    "  add a0, a0, t5\n"
    "  sd t0, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_sip_before_clear\n"
    "  add a0, a0, t5\n"
    "  sd t6, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_marker_addr\n"
    "  ld a0, 0(a0)\n"
    "  li a1, 0\n"
    "  beqz a0, .Lai_imv_clear_retry_marker_before_done\n"
    "  ld a1, 0(a0)\n"
    ".Lai_imv_clear_retry_marker_before_done:\n"
    "  la a0, ai_imv_clear_retry_marker_before\n"
    "  add a0, a0, t5\n"
    "  sd a1, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_log_addr\n"
    "  ld a0, 0(a0)\n"
    "  beqz a0, .Lai_imv_clear_retry_log_before_clear_done\n"
    "  li a1, 0x696d7673636c5230\n"
    "  add a1, a1, t3\n"
    "  sd a1, 0(a0)\n"
    ".Lai_imv_clear_retry_log_before_clear_done:\n"
    "  la a0, ai_imv_clear_retry_fault_count\n"
    "  ld a1, 0(a0)\n"
    "  la a2, ai_imv_clear_retry_faults_seen_at_clear\n"
    "  sd a1, 0(a2)\n"
    "  li a0, 0x40601000\n"
    "  sd zero, 0(a0)\n"
    "  csrr t6, sip\n"
    "  la a0, ai_imv_clear_retry_sip_after_clear\n"
    "  add a0, a0, t5\n"
    "  sd t6, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_marker_addr\n"
    "  ld a0, 0(a0)\n"
    "  li a1, 0\n"
    "  beqz a0, .Lai_imv_clear_retry_marker_after_done\n"
    "  ld a1, 0(a0)\n"
    ".Lai_imv_clear_retry_marker_after_done:\n"
    "  la a0, ai_imv_clear_retry_marker_after\n"
    "  add a0, a0, t5\n"
    "  sd a1, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_clear_success_count\n"
    "  ld a1, 0(a0)\n"
    "  addi a1, a1, 1\n"
    "  sd a1, 0(a0)\n"
    "  addi t3, t3, 1\n"
    "  sd t3, 0(t2)\n"
    "  j .Lai_imv_clear_retry_restore\n"
    ".Lai_imv_clear_retry_clear_source:\n"
    "  li a0, 0x40601000\n"
    "  sd zero, 0(a0)\n"
    "  j .Lai_imv_clear_retry_restore\n"
    ".Lai_imv_clear_retry_sync_fault:\n"
    "  li t2, 15\n"
    "  bne t0, t2, .Lai_imv_clear_retry_restore\n"
    "  la t2, ai_imv_clear_retry_fault_count\n"
    "  ld t3, 0(t2)\n"
    "  li t4, 3\n"
    "  bgeu t3, t4, .Lai_imv_clear_retry_restore\n"
    "  addi t5, t3, 1\n"
    "  slli t5, t5, 3\n"
    "  la t6, ai_imv_clear_retry_scause\n"
    "  add t6, t6, t5\n"
    "  sd t0, 0(t6)\n"
    "  la t6, ai_imv_clear_retry_stval\n"
    "  add t6, t6, t5\n"
    "  sd t1, 0(t6)\n"
    "  csrr t6, sip\n"
    "  la a0, ai_imv_clear_retry_sip_on_fault\n"
    "  add a0, a0, t5\n"
    "  sd t6, 0(a0)\n"
    "  la a0, ai_imv_clear_retry_store_pte_addr\n"
    "  ld a0, 0(a0)\n"
    "  beqz a0, .Lai_imv_clear_retry_fault_counted\n"
    "  la a1, ai_imv_clear_retry_store_pte_value\n"
    "  ld a1, 0(a1)\n"
    "  sd a1, 0(a0)\n"
    "  sfence.vma\n"
    ".Lai_imv_clear_retry_fault_counted:\n"
    "  addi t3, t3, 1\n"
    "  sd t3, 0(t2)\n"
    ".Lai_imv_clear_retry_restore:\n"
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
    "  sret\n");

static inline void ai_imv_reset_descriptor_log(void)
{
    ai_imv_trap_count = 0ULL;
    ai_imv_desc_ring_base_addr = 0ULL;
    ai_imv_desc_tail_addr = 0ULL;
    ai_imv_desc_ack_base_addr = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMV_LOG_SLOTS; ++i) {
        ai_imv_trap_cause[i] = 0ULL;
        ai_imv_trap_mip[i] = 0ULL;
        ai_imv_desc_seq_log[i] = 0ULL;
        ai_imv_desc_payload_log[i] = 0ULL;
        ai_imv_desc_checksum_log[i] = 0ULL;
        ai_imv_desc_tail_log[i] = 0ULL;
        ai_imv_desc_ack_log[i] = 0ULL;
        ai_imv_source_clear_log[i] = 0ULL;
    }
}

static inline uint64_t ai_imv_spin_until_desc_count(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && ai_imv_trap_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline void ai_imv_vmodule_write_reg(uint64_t offset, uint64_t data)
{
    const uint64_t addr_cmd = 0xffff000100000000ULL |
        (offset & 0xffffffffULL);
    const uint64_t data_cmd = 0xffff000000000000ULL |
        (data & 0xffffffffULL);

    asm volatile(
        "mv t0, %[addr]\n\t"
        "mv t1, %[data]\n\t"
        "fence.i\n\t"
        "add x0, x0, t0\n\t"
        "fence.i\n\t"
        "add x0, x0, t1\n\t"
        "fence.i\n\t"
        :
        : [addr] "r"(addr_cmd),
          [data] "r"(data_cmd)
        : "t0", "t1", "memory");
}

static inline void ai_imv_vmodule_clear_main_interrupt(void)
{
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT, 0ULL);
}

static inline void ai_imv_vmodule_clear_error_sources(void)
{
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_CHI_ERROR, 0ULL);
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_AXI_ERROR, 0ULL);
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_FORCE_NMI, 0ULL);
}

static inline void ai_imv_vmodule_set_delay(uint64_t delay)
{
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_INTERRUPT_DELAY, delay);
}

static inline uint64_t ai_imv_satp_mode(void)
{
#ifdef sv48
    return 0x9ULL << 60;
#else
    return 0x8ULL << 60;
#endif
}

static inline pte_t *ai_imv_hs_leaf_entry(enum test_page page)
{
#if defined(sv39)
    return &hspt[2][page];
#else
    return &hspt[4][page];
#endif
}

static inline void ai_imv_set_hs_leaf_to_paddr(enum test_page page,
    uintptr_t paddr,
    pte_t flags)
{
    *ai_imv_hs_leaf_entry(page) =
        ((paddr & PAGE_ADDR_MSK) >> 2U) | flags;
    fence_iorw_iorw();
    sfence_vma();
}

static inline void ai_imv_map_vmodule_mmio_window(uintptr_t mmio_addr,
    pte_t leaf_flags)
{
    for (uint64_t i = 0ULL; i < PAGE_SIZE / sizeof(pte_t); ++i) {
        ai_imv_vmodule_mmio_l2[i] = 0ULL;
        ai_imv_vmodule_mmio_l3[i] = 0ULL;
    }

#ifdef sv48
    hspt[1][PTE_INDEX(1, mmio_addr)] =
        PTE_V | (((uintptr_t)ai_imv_vmodule_mmio_l2) >> 2U);
    ai_imv_vmodule_mmio_l2[PTE_INDEX(2, mmio_addr)] =
        PTE_V | (((uintptr_t)ai_imv_vmodule_mmio_l3) >> 2U);
    ai_imv_vmodule_mmio_l3[PTE_INDEX(3, mmio_addr)] =
        ((mmio_addr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#else
    hspt[0][PTE_INDEX(0, mmio_addr)] =
        PTE_V | (((uintptr_t)ai_imv_vmodule_mmio_l2) >> 2U);
    ai_imv_vmodule_mmio_l2[PTE_INDEX(1, mmio_addr)] =
        PTE_V | (((uintptr_t)ai_imv_vmodule_mmio_l3) >> 2U);
    ai_imv_vmodule_mmio_l3[PTE_INDEX(2, mmio_addr)] =
        ((mmio_addr & PAGE_ADDR_MSK) >> 2U) | leaf_flags;
#endif
    fence_iorw_iorw();
    sfence_vma();
}

static inline void ai_imv_prepare_vmodule_sei_in_hs(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRS(CSR_MIDELEG, SIE_SEIE);
    CSRC(CSR_MIP, SIP_SEIP);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    CSRW(CSR_SIE, SIE_SEIE);
    CSRS(CSR_SSTATUS, SSTATUS_SIE_BIT);
}

static inline void ai_imv_prepare_vmodule_mei(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MIP, 0ULL);
    CSRW(CSR_MIE, MIE_MEIE);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
}

static inline void ai_imv_restore_m_interrupt_state(uint64_t saved_mstatus,
    uint64_t saved_mie, uint64_t saved_mideleg)
{
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_imv_vmodule_clear_main_interrupt();
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_imv_reset_clear_retry_log(void)
{
    ai_imv_clear_retry_interrupt_count = 0ULL;
    ai_imv_clear_retry_fault_count = 0ULL;
    ai_imv_clear_retry_clear_success_count = 0ULL;
    ai_imv_clear_retry_faults_seen_at_clear = 0ULL;
    ai_imv_clear_retry_marker_addr = 0ULL;
    ai_imv_clear_retry_log_addr = 0ULL;
    ai_imv_clear_retry_store_pte_addr = 0ULL;
    ai_imv_clear_retry_store_pte_value = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMV_LOG_SLOTS; ++i) {
        ai_imv_clear_retry_scause[i] = 0ULL;
        ai_imv_clear_retry_stval[i] = 0ULL;
        ai_imv_clear_retry_sip_before_clear[i] = 0ULL;
        ai_imv_clear_retry_sip_on_fault[i] = 0ULL;
        ai_imv_clear_retry_sip_after_clear[i] = 0ULL;
        ai_imv_clear_retry_marker_before[i] = 0ULL;
        ai_imv_clear_retry_marker_after[i] = 0ULL;
    }
}

static inline uint64_t ai_imv_spin_until_clear_retry_interrupt(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit &&
        ai_imv_clear_retry_interrupt_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline void ai_imv_reset_error_log(void)
{
    ai_imv_error_rnmi_count = 0ULL;
    ai_imv_error_mnret_markers = 0ULL;
    ai_imv_error_ordinary_count = 0ULL;
    ai_imv_error_old_addr = 0ULL;
    ai_imv_error_marker_addr = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMV_LOG_SLOTS; ++i) {
        ai_imv_error_rnmi_mncause[i] = 0ULL;
        ai_imv_error_rnmi_mnepc[i] = 0ULL;
        ai_imv_error_rnmi_mnstatus[i] = 0ULL;
        ai_imv_error_rnmi_mip[i] = 0ULL;
        ai_imv_error_rnmi_old_value[i] = 0ULL;
        ai_imv_error_rnmi_marker_value[i] = 0ULL;
        ai_imv_error_ordinary_cause[i] = 0ULL;
        ai_imv_error_ordinary_mip[i] = 0ULL;
        ai_imv_error_ordinary_marker_value[i] = 0ULL;
    }
}

static inline void ai_imv_prepare_error_rnmi_mei_state(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRW(CSR_MIDELEG, 0ULL);
    CSRW(CSR_MIP, 0ULL);
    CSRW(CSR_MIE, MIE_MEIE);
    rnmi_handler_enabled = true;
    CSRS(CSR_MNSTATUS, MNSTATUS_NMIE);
}

static inline void ai_imv_restore_error_state(uint64_t saved_mstatus,
    uint64_t saved_mie, uint64_t saved_mideleg, uint64_t saved_mnstatus)
{
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    rnmi_handler_enabled = false;
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MNSTATUS, saved_mnstatus);
    CSRW(CSR_MSTATUS, saved_mstatus);
}

static inline void ai_imv_reset_window_log(void)
{
    ai_imv_window_ordinary_count = 0ULL;
    ai_imv_window_rnmi_count = 0ULL;
    ai_imv_window_rnmi_mnret_markers = 0ULL;
    ai_imv_window_marker_addr = 0ULL;
    ai_imv_window_ordinary_log_addr = 0ULL;
    ai_imv_window_rnmi_log_addr = 0ULL;
    for (uint64_t i = 0ULL; i < AI_IMV_LOG_SLOTS; ++i) {
        ai_imv_window_ordinary_cause[i] = 0ULL;
        ai_imv_window_ordinary_mip_before[i] = 0ULL;
        ai_imv_window_ordinary_mip_after_rnmi[i] = 0ULL;
        ai_imv_window_ordinary_mip_after_clear[i] = 0ULL;
        ai_imv_window_ordinary_marker_before[i] = 0ULL;
        ai_imv_window_ordinary_marker_after_rnmi[i] = 0ULL;
        ai_imv_window_ordinary_clear_log[i] = 0ULL;
        ai_imv_window_rnmi_mncause[i] = 0ULL;
        ai_imv_window_rnmi_mnstatus[i] = 0ULL;
        ai_imv_window_rnmi_mip[i] = 0ULL;
        ai_imv_window_rnmi_marker_value[i] = 0ULL;
    }
}

static inline uint64_t ai_imv_spin_until_window_ordinary(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit &&
        ai_imv_window_ordinary_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline uint64_t ai_imv_spin_until_window_rnmi(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && ai_imv_window_rnmi_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline void ai_imv_restore_pmp_full_access(void)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, AI_IMV_PMP_TOR_RWX);
    CSRW(CSR_PMPADDR0, (uint64_t)-1);
    CSRW(CSR_PMPADDR1, 0ULL);
    CSRW(CSR_PMPADDR2, 0ULL);
}

static inline void ai_imv_restore_pmp_state(uint64_t saved_pmpcfg0,
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

static inline uint64_t ai_imv_pmp_window_cfg(uint64_t middle_cfg)
{
    return AI_IMV_PMP_TOR_RWX | (middle_cfg << 8U) |
        (AI_IMV_PMP_TOR_RWX << 16U);
}

static inline void ai_imv_configure_pmp_window(uintptr_t start,
    uintptr_t end,
    uint64_t middle_cfg)
{
    goto_priv(PRIV_M);
    CSRW(CSR_PMPCFG0, 0ULL);
    CSRW(CSR_PMPADDR0, start >> 2U);
    CSRW(CSR_PMPADDR1, end >> 2U);
    CSRW(CSR_PMPADDR2, (uint64_t)-1);
    CSRW(CSR_PMPCFG0, ai_imv_pmp_window_cfg(middle_cfg));
}

static inline void ai_imv_prepare_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    hspt_init();
    hspt_u_mode_allow();
    sfence_vma();
}

static inline void ai_imv_enable_mprv_user_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IMV_MSTATUS_MPP_MASK);
    CSRS(CSR_MSTATUS, MSTATUS_MPRV);
}

static inline void ai_imv_clear_mprv_translation(void)
{
    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MPRV | AI_IMV_MSTATUS_MPP_MASK);
}

static inline uint64_t ai_imv_ld_probe(uintptr_t addr, uint64_t sentinel)
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

static inline uint64_t ai_imv_spin_until_trap(uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && !excpt.triggered; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline bool ai_imv_bytes_match(uintptr_t addr,
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

static inline bool ai_imv_is_error_nmi_cause(uint64_t cause)
{
    const uint64_t code = cause & CAUSE_MSK;

    return (cause & CAUSE_INT_BIT) != 0ULL &&
        (code == 31ULL || code == 43ULL);
}

static inline uint64_t ai_imv_spin_until_error_rnmi(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit && ai_imv_error_rnmi_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

static inline uint64_t ai_imv_spin_until_error_ordinary(uint64_t expected,
    uint64_t limit)
{
    uint64_t rounds = 0ULL;

    for (; rounds < limit &&
        ai_imv_error_ordinary_count < expected; ++rounds) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    return rounds;
}

bool ai_interrupt_memory_m_vmodule_chi_error_rnmi_mei_marker_owner_corner()
{
    TEST_START();

    const uintptr_t old_addr = phys_page_base(VSRWX_GRWX) + 0x900U;
    const uintptr_t new_addr = phys_page_base(VSRWX_GRWX) + 0x908U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0x910U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0x918U;
    const uint64_t old_seed = 0x696d766368690001ULL;
    const uint64_t new_seed = 0x696d766368690002ULL;
    const uint64_t new_value = 0x696d766368690102ULL;
    const uint64_t marker_before = 0x696d7663686900f0ULL;
    const uint64_t marker_after = 0x696d7663686900f1ULL;
    const uint64_t guard_seed = 0x696d7663686900aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t rnmi_rounds = 0ULL;
    uint64_t mei_rounds = 0ULL;
    uint64_t old_before = 0ULL;
    uint64_t marker_after_rnmi = 0ULL;
    uint64_t final_mip = 0ULL;

    write64(old_addr, old_seed);
    write64(new_addr, new_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_reset_error_log();
    ai_imv_error_old_addr = old_addr;
    ai_imv_error_marker_addr = marker_addr;
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_error_rnmi_mei_handler);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_error_rnmi_mei_state();

    old_before = read64(old_addr);
    write64(marker_addr, marker_before);
    fence_iorw_iorw();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_CHI_ERROR, 1ULL);
    rnmi_rounds = ai_imv_spin_until_error_rnmi(1ULL, AI_IMV_SPIN_LIMIT);
    marker_after_rnmi = read64(marker_addr);

    write64(new_addr, new_value);
    fence_iorw_iorw();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    mei_rounds = ai_imv_spin_until_error_ordinary(1ULL, AI_IMV_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    final_mip = CSRR(CSR_MIP);
    write64(marker_addr, marker_after);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < AI_IMV_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool error_rnmi_is_single_owner =
        rnmi_rounds < AI_IMV_SPIN_LIMIT &&
        old_before == old_seed &&
        ai_imv_error_rnmi_count == 1ULL &&
        ai_imv_error_mnret_markers == 1ULL &&
        ai_imv_is_error_nmi_cause(ai_imv_error_rnmi_mncause[0]) &&
        (ai_imv_error_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_imv_error_rnmi_old_value[0] == old_seed &&
        ai_imv_error_rnmi_marker_value[0] == marker_before &&
        marker_after_rnmi == marker_before;
    const bool ordinary_interrupt_remains_separate =
        mei_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_error_ordinary_count == 1ULL &&
        ai_imv_error_ordinary_cause[0] == CAUSE_MEI &&
        ai_imv_error_ordinary_marker_value[0] == marker_before &&
        ai_imv_error_rnmi_count == 1ULL &&
        ai_imv_error_mnret_markers == 1ULL &&
        (final_mip & MIP_MEIP) == 0ULL;
    const bool post_error_memory_owner_is_clean =
        read64(old_addr) == old_seed &&
        read64(new_addr) == new_value &&
        read64(marker_addr) == marker_after &&
        read64(guard_addr) == guard_seed &&
        ai_imv_error_rnmi_count == 1ULL &&
        ai_imv_error_ordinary_count == 1ULL;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_error_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mnstatus);
    ai_imv_error_old_addr = 0ULL;
    ai_imv_error_marker_addr = 0ULL;

    TEST_ASSERT("VModule CHI error special-run records exactly one RNMI/error event and keeps the pre-error memory owner visible in handler state",
        error_rnmi_is_single_owner,
        "rounds=%lx rnmi_count=%lx mncause=%lx mnstatus=%lx old=%lx marker=%lx marker_after_rnmi=%lx",
        rnmi_rounds, ai_imv_error_rnmi_count,
        ai_imv_error_rnmi_mncause[0],
        ai_imv_error_rnmi_mnstatus[0],
        ai_imv_error_rnmi_old_value[0],
        ai_imv_error_rnmi_marker_value[0],
        marker_after_rnmi);
    TEST_ASSERT("a later ordinary VModule MEI handler is observed as a separate source and does not overwrite the RNMI/error owner log",
        ordinary_interrupt_remains_separate,
        "mei_rounds=%lx ordinary_count=%lx cause=%lx rnmi_count=%lx mnret=%lx mip=%lx",
        mei_rounds, ai_imv_error_ordinary_count,
        ai_imv_error_ordinary_cause[0], ai_imv_error_rnmi_count,
        ai_imv_error_mnret_markers, final_mip);
    TEST_ASSERT("post-error correct-path memory load/store state is not polluted by the earlier error/RNMI or MEI source-clear paths",
        post_error_memory_owner_is_clean,
        "old=%lx new=%lx marker=%lx guard=%lx rnmi=%lx ordinary=%lx",
        read64(old_addr), read64(new_addr), read64(marker_addr),
        read64(guard_addr), ai_imv_error_rnmi_count,
        ai_imv_error_ordinary_count);

    TEST_END(ai_interrupt_memory_m_vmodule_chi_error_rnmi_mei_marker_owner_corner);
}

bool ai_interrupt_memory_m_vmodule_axi_error_rnmi_store_marker_owner_corner()
{
    TEST_START();

    const uintptr_t old_addr = phys_page_base(VSRWX_GRWX) + 0x980U;
    const uintptr_t store_addr = phys_page_base(VSRWX_GRWX) + 0x988U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0x990U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0x998U;
    const uint64_t old_seed = 0x696d766178690001ULL;
    const uint64_t store_seed = 0x696d766178690002ULL;
    const uint64_t store_value0 = 0x696d766178690102ULL;
    const uint64_t store_value1 = 0x696d766178690202ULL;
    const uint64_t marker_before = 0x696d7661786900e0ULL;
    const uint64_t marker_after = 0x696d7661786900e1ULL;
    const uint64_t guard_seed = 0x696d7661786900aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t rnmi_rounds = 0ULL;
    uint64_t mei_rounds = 0ULL;
    uint64_t final_mip = 0ULL;

    write64(old_addr, old_seed);
    write64(store_addr, store_seed);
    write64(marker_addr, marker_before);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_reset_error_log();
    ai_imv_error_old_addr = old_addr;
    ai_imv_error_marker_addr = marker_addr;
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_error_rnmi_mei_handler);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_error_rnmi_mei_state();

    write64(store_addr, store_value0);
    fence_iorw_iorw();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_AXI_ERROR, 1ULL);
    rnmi_rounds = ai_imv_spin_until_error_rnmi(1ULL, AI_IMV_SPIN_LIMIT);
    write64(store_addr, store_value1);
    fence_iorw_iorw();

    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    mei_rounds = ai_imv_spin_until_error_ordinary(1ULL, AI_IMV_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    final_mip = CSRR(CSR_MIP);
    write64(marker_addr, marker_after);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < AI_IMV_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool axi_error_rnmi_once =
        rnmi_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_error_rnmi_count == 1ULL &&
        ai_imv_error_mnret_markers == 1ULL &&
        ai_imv_is_error_nmi_cause(ai_imv_error_rnmi_mncause[0]) &&
        (ai_imv_error_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        ai_imv_error_rnmi_old_value[0] == old_seed &&
        ai_imv_error_rnmi_marker_value[0] == marker_before;
    const bool store_marker_not_lost_or_duplicated =
        read64(store_addr) == store_value1 &&
        read64(marker_addr) == marker_after &&
        read64(guard_addr) == guard_seed;
    const bool later_mei_is_independent =
        mei_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_error_ordinary_count == 1ULL &&
        ai_imv_error_ordinary_cause[0] == CAUSE_MEI &&
        ai_imv_error_ordinary_marker_value[0] == marker_before &&
        ai_imv_error_rnmi_count == 1ULL &&
        ai_imv_error_mnret_markers == 1ULL &&
        (final_mip & MIP_MEIP) == 0ULL;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_error_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mnstatus);
    ai_imv_error_old_addr = 0ULL;
    ai_imv_error_marker_addr = 0ULL;

    TEST_ASSERT("VModule AXI error special-run records a single RNMI/error event without reclassifying it as ordinary MEI",
        axi_error_rnmi_once,
        "rounds=%lx rnmi_count=%lx mncause=%lx mnstatus=%lx old=%lx marker=%lx",
        rnmi_rounds, ai_imv_error_rnmi_count,
        ai_imv_error_rnmi_mncause[0],
        ai_imv_error_rnmi_mnstatus[0],
        ai_imv_error_rnmi_old_value[0],
        ai_imv_error_rnmi_marker_value[0]);
    TEST_ASSERT("stores before and after the AXI-error RNMI window converge to the final correct-path image exactly once",
        store_marker_not_lost_or_duplicated,
        "store=%lx marker=%lx guard=%lx",
        read64(store_addr), read64(marker_addr), read64(guard_addr));
    TEST_ASSERT("the later VModule MEI source clear remains separate from the earlier AXI-error RNMI state",
        later_mei_is_independent,
        "mei_rounds=%lx ordinary=%lx cause=%lx rnmi=%lx mnret=%lx mip=%lx",
        mei_rounds, ai_imv_error_ordinary_count,
        ai_imv_error_ordinary_cause[0], ai_imv_error_rnmi_count,
        ai_imv_error_mnret_markers, final_mip);

    TEST_END(ai_interrupt_memory_m_vmodule_axi_error_rnmi_store_marker_owner_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_load_store_source_clear_corner()
{
    TEST_START();

    const uintptr_t pre_addr = phys_page_base(VSRWX_GRWX) + 0x540U;
    const uintptr_t load_addr = phys_page_base(VSRWX_GRWX) + 0x548U;
    const uintptr_t store_addr = phys_page_base(VSRWX_GRWX) + 0x550U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0x558U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0x560U;
    const uint64_t pre_seed = 0x696d63766d656901ULL;
    const uint64_t pre_value = 0x696d63766d656911ULL;
    const uint64_t load_seed = 0x696d63766d656902ULL;
    const uint64_t store_seed = 0x696d63766d656903ULL;
    const uint64_t store_value = 0x696d63766d656922ULL;
    const uint64_t marker_value = 0x696d63766d656933ULL;
    const uint64_t guard_seed = 0x696d63766d6569aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t load_observed = 0ULL;
    uint64_t rounds = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(pre_addr, pre_seed);
    write64(load_addr, load_seed);
    write64(store_addr, store_seed);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(16ULL);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();

    write64(pre_addr, pre_value);
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    load_observed = read64(load_addr);
    write64(store_addr, store_value);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();
    rounds = ai_imv_spin_until_trap(AI_IMV_SPIN_LIMIT);

    const bool mei_seen =
        rounds < AI_IMV_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MEI;
    mip_after_trap = CSRR(CSR_MIP);

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    ai_imv_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("VModule MEI is observed through the external injection path and cleared by the handler",
        mei_seen &&
        (mip_after_trap & MIP_MEIP) == 0ULL);
    TEST_ASSERT("mainline load/store progress around delayed VModule MEI remains exactly-once",
        load_observed == load_seed &&
        read64(pre_addr) == pre_value &&
        read64(store_addr) == store_value &&
        read64(marker_addr) == marker_value);
    TEST_ASSERT("VModule MEI handling does not overwrite the independent guard word",
        read64(guard_addr) == guard_seed);

    TEST_END(ai_interrupt_memory_m_vmodule_mei_load_store_source_clear_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_store_drain_post_store_image_corner()
{
    TEST_START();

    static const uint8_t expected_post_bytes[16] = {
        0x11U, 0x55U, 0x66U, 0x44U,
        0xddU, 0xccU, 0xbbU, 0xaaU,
        0x99U, 0x77U, 0x88U, 0xccU,
        0xddU, 0xeeU, 0xffU, 0x22U,
    };
    const uintptr_t drain0 = phys_page_base(VSRWX_GRWX) + 0x600U;
    const uintptr_t drain1 = phys_page_base(VSRWX_GRWX) + 0x608U;
    const uintptr_t drain2 = phys_page_base(VSRWX_GRWX) + 0x610U;
    const uintptr_t post_base = phys_page_base(VSRWX_GRWX) + 0x620U;
    const uintptr_t marker = phys_page_base(VSRWX_GRWX) + 0x638U;
    const uintptr_t guard = phys_page_base(VSRWX_GRWX) + 0x640U;
    const uint64_t drain0_value = 0x696d7664726e0001ULL;
    const uint64_t drain1_value = 0x696d7664726e0002ULL;
    const uint64_t drain2_value = 0x696d7664726e0003ULL;
    const uint64_t post_seed = 0x2211ffeeddccbbaaULL;
    const uint64_t post_seed_hi = 0xddccbbaa99887766ULL;
    const uint64_t marker_value = 0x696d7664726e00f1ULL;
    const uint64_t guard_seed = 0x696d7664726e00aaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    uint64_t rounds = 0ULL;
    uint64_t mip_after_trap = 0ULL;

    write64(drain0, 0ULL);
    write64(drain1, 0ULL);
    write64(drain2, 0ULL);
    write64(post_base, post_seed);
    write64(post_base + sizeof(uint64_t), post_seed_hi);
    write64(marker, 0ULL);
    write64(guard, guard_seed);
    fence_iorw_iorw();

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(8ULL);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();

    write64(drain0, drain0_value);
    write64(drain1, drain1_value);
    write64(drain2, drain2_value);
    fence_iorw_iorw();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    rounds = ai_imv_spin_until_trap(AI_IMV_SPIN_LIMIT);
    mip_after_trap = CSRR(CSR_MIP);

    sb(post_base + 1U, 0x55U);
    sh(post_base + 2U, 0x4466U);
    sw(post_base + 4U, 0xaabbccddU);
    sh(post_base + 10U, 0xcc88U);
    write64(marker, marker_value);
    fence_iorw_iorw();

    const bool mei_seen_and_cleared =
        rounds < AI_IMV_SPIN_LIMIT &&
        excpt.triggered &&
        excpt.priv == PRIV_M &&
        excpt.cause == CAUSE_MEI &&
        (mip_after_trap & MIP_MEIP) == 0ULL;
    const bool drain_image_preserved =
        read64(drain0) == drain0_value &&
        read64(drain1) == drain1_value &&
        read64(drain2) == drain2_value;
    const bool post_handler_store_image =
        ai_imv_bytes_match(post_base, expected_post_bytes,
            sizeof(expected_post_bytes)) &&
        read64(marker) == marker_value &&
        read64(guard) == guard_seed;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    ai_imv_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);

    TEST_ASSERT("VModule MEI is delivered after the store-drain pressure sequence and its source is cleared",
        mei_seen_and_cleared);
    TEST_ASSERT("stores issued before the delayed VModule MEI keep their independent drain image",
        drain_image_preserved);
    TEST_ASSERT("the first post-handler byte/half/word stores use current data/mask/addr and preserve guard",
        post_handler_store_image);

    TEST_END(ai_interrupt_memory_m_vmodule_mei_store_drain_post_store_image_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_load_refill_same_rd_owner_audit_corner()
{
    TEST_START();

    const uintptr_t old_addr = phys_page_base(VSRWX_GRWX) + 0xb00U;
    const uintptr_t new_addr = phys_page_base(VSRWX_GRWX) + 0xb40U;
    const uintptr_t desc_base = phys_page_base(VSRWX_GRWX) + 0xb80U;
    const uintptr_t desc_tail_addr = phys_page_base(VSRWX_GRWX) + 0xba0U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0xba8U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0xbb0U;
    const uint64_t old_seed = 0x696d7672666f6c64ULL;
    const uint64_t new_seed = 0x696d7672666e6577ULL;
    const uint64_t handler_seq = 0x696d76726668646cULL;
    const uint64_t handler_payload = 0x696d767266686470ULL;
    const uint64_t handler_checksum = 0x696d76726668636bULL;
    const uint64_t marker_value = 0x696d7672666d6b21ULL;
    const uint64_t guard_seed = 0x696d767266677564ULL;
    const uint64_t poisoned_rd = 0x696d767266706f69ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mepc = CSRR(CSR_MEPC);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    uint64_t old_load_rd = poisoned_rd;
    uint64_t new_load_rd = poisoned_rd;
    uint64_t replay_load_rd = poisoned_rd;
    uint64_t rounds = 0ULL;
    uint64_t mip_after_handler = 0ULL;

    goto_priv(PRIV_M);
    write64(old_addr, old_seed);
    write64(new_addr, new_seed);
    write64(desc_base + 0U, handler_seq);
    write64(desc_base + 8U, handler_payload);
    write64(desc_base + 16U, handler_checksum);
    write64(desc_tail_addr, 0ULL);
    write64(marker_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();
    CSRW(CSR_MENVCFG, (saved_menvcfg & ~MENVCFG_CBIE11) |
        MENVCFG_CBIE01);

    ai_imv_reset_descriptor_log();
    ai_imv_desc_ring_base_addr = desc_base;
    ai_imv_desc_tail_addr = desc_tail_addr;
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(8ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_descriptor_ack_handler);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();

    cbo_inval(old_addr);
    cbo_inval(new_addr);
    fence_iorw_iorw();
    old_load_rd = read64(old_addr);
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    new_load_rd = read64(new_addr);
    rounds = ai_imv_spin_until_desc_count(1ULL, AI_IMV_SPIN_LIMIT);
    mip_after_handler = CSRR(CSR_MIP);
    replay_load_rd = read64(new_addr);
    write64(marker_addr, marker_value);
    fence_iorw_iorw();

    const bool vmodule_mei_delivered_once =
        rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_trap_count == 1ULL &&
        ai_imv_trap_cause[0] == CAUSE_MEI &&
        (ai_imv_trap_mip[0] & MIP_MEIP) != 0ULL &&
        ai_imv_source_clear_log[0] == 1ULL &&
        (mip_after_handler & MIP_MEIP) == 0ULL &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL;
    const bool load_refill_payloads_keep_same_rd_owner =
        !excpt.triggered &&
        old_load_rd == old_seed &&
        new_load_rd == new_seed &&
        replay_load_rd == new_seed &&
        old_load_rd != new_load_rd &&
        old_load_rd != poisoned_rd &&
        new_load_rd != poisoned_rd &&
        read64(old_addr) == old_seed &&
        read64(new_addr) == new_seed;
    const bool handler_and_marker_memory_are_disjoint =
        read64(desc_base + 0U) == handler_seq &&
        read64(desc_base + 8U) == handler_payload &&
        read64(desc_base + 16U) == handler_checksum &&
        read64(desc_tail_addr) == 1ULL &&
        read64(marker_addr) == marker_value &&
        read64(guard_addr) == guard_seed &&
        ai_imv_desc_ring_base_addr == desc_base &&
        ai_imv_desc_tail_addr == desc_tail_addr &&
        ai_imv_desc_seq_log[0] == handler_seq &&
        ai_imv_desc_payload_log[0] == handler_payload &&
        ai_imv_desc_checksum_log[0] == handler_checksum &&
        ai_imv_desc_ack_log[0] == 0ULL;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);
    CSRW(CSR_MEPC, saved_mepc);
    CSRW(CSR_MENVCFG, saved_menvcfg);
    ai_imv_desc_ring_base_addr = 0ULL;
    ai_imv_desc_tail_addr = 0ULL;
    ai_imv_desc_ack_base_addr = 0ULL;

    TEST_ASSERT("VModule delayed MEI is delivered and cleared once in the cache/refill candidate load window",
        vmodule_mei_delivered_once,
        "rounds=%lx trap_count=%lx cause=%lx mip0=%lx source_clear=%lx mip_after=%lx final_mip=%lx",
        rounds, ai_imv_trap_count, ai_imv_trap_cause[0],
        ai_imv_trap_mip[0], ai_imv_source_clear_log[0],
        mip_after_handler, CSRR(CSR_MIP));
    TEST_ASSERT("old-path and correct-path cacheline loads reuse the same C destination variable without late-response payload leakage",
        load_refill_payloads_keep_same_rd_owner,
        "old_rd=%lx new_rd=%lx replay=%lx old_mem=%lx new_mem=%lx poison=%lx",
        old_load_rd, new_load_rd, replay_load_rd, read64(old_addr),
        read64(new_addr), poisoned_rd);
    TEST_ASSERT("VModule handler memory, mainline marker and guard remain separate from the two load payload owners",
        handler_and_marker_memory_are_disjoint,
        "handler=%lx desc_seq=%lx desc_payload=%lx tail=%lx marker=%lx guard=%lx",
        read64(desc_base), ai_imv_desc_seq_log[0],
        ai_imv_desc_payload_log[0], read64(marker_addr),
        read64(desc_tail_addr), read64(guard_addr));

    TEST_END(ai_interrupt_memory_m_vmodule_mei_load_refill_same_rd_owner_audit_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_descriptor_doorbell_payload_ack_corner()
{
    TEST_START();

    const uintptr_t desc_base = phys_page_base(VSRWX_GRWX) + 0x700U;
    const uintptr_t tail_addr = phys_page_base(VSRWX_GRWX) + 0x780U;
    const uintptr_t ack_base = phys_page_base(VSRWX_GRWX) + 0x788U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0x7a0U;
    const uintptr_t post_addr = phys_page_base(VSRWX_GRWX) + 0x7a8U;
    const uint64_t seq = 0x25ULL;
    const uint64_t payload = 0x696d766464657363ULL;
    const uint64_t checksum = payload ^ (seq << 8U) ^
        0x55aa55aa55aa55aaULL;
    const uint64_t stale_seq = 0x10ULL;
    const uint64_t stale_payload = 0x696d76647374616cULL;
    const uint64_t guard_seed = 0x696d7664646573aaULL;
    const uint64_t post_value = 0x696d766464657370ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    uint64_t rounds = 0ULL;
    uint64_t mip_after_handler = 0ULL;

    write64(desc_base + 0U, stale_seq);
    write64(desc_base + 8U, stale_payload);
    write64(desc_base + 16U, stale_payload ^ stale_seq);
    write64(desc_base + 24U, 0ULL);
    write64(tail_addr, 0ULL);
    write64(ack_base, 0ULL);
    write64(guard_addr, guard_seed);
    write64(post_addr, 0ULL);
    fence_iorw_iorw();

    ai_imv_reset_descriptor_log();
    ai_imv_desc_ring_base_addr = desc_base;
    ai_imv_desc_tail_addr = tail_addr;
    ai_imv_desc_ack_base_addr = ack_base;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_descriptor_ack_handler);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(12ULL);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();

    write64(desc_base + 8U, payload);
    write64(desc_base + 16U, checksum);
    write64(desc_base + 0U, seq);
    fence_iorw_iorw();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    rounds = ai_imv_spin_until_desc_count(1ULL, AI_IMV_SPIN_LIMIT);
    mip_after_handler = CSRR(CSR_MIP);
    write64(post_addr, post_value);
    fence_iorw_iorw();

    const bool doorbell_payload_ack_same_generation =
        rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_trap_count == 1ULL &&
        ai_imv_trap_cause[0] == CAUSE_MEI &&
        ai_imv_desc_tail_log[0] == 0ULL &&
        ai_imv_desc_seq_log[0] == seq &&
        ai_imv_desc_payload_log[0] == payload &&
        ai_imv_desc_payload_log[0] != stale_payload &&
        ai_imv_desc_checksum_log[0] == checksum &&
        ai_imv_desc_checksum_log[0] ==
            (ai_imv_desc_payload_log[0] ^
             (ai_imv_desc_seq_log[0] << 8U) ^
             0x55aa55aa55aa55aaULL) &&
        ai_imv_desc_ack_log[0] == seq &&
        read64(ack_base) == seq &&
        read64(tail_addr) == 1ULL;
    const bool source_clear_and_memory_disjoint =
        ai_imv_source_clear_log[0] == 1ULL &&
        (mip_after_handler & MIP_MEIP) == 0ULL &&
        read64(desc_base + 0U) == seq &&
        read64(desc_base + 8U) == payload &&
        read64(desc_base + 16U) == checksum &&
        read64(guard_addr) == guard_seed &&
        read64(post_addr) == post_value;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);
    ai_imv_desc_ring_base_addr = 0ULL;
    ai_imv_desc_tail_addr = 0ULL;
    ai_imv_desc_ack_base_addr = 0ULL;

    TEST_ASSERT("VModule MEI doorbell handler consumes the fenced descriptor payload/checksum generation and writes matching ack",
        doorbell_payload_ack_same_generation);
    TEST_ASSERT("VModule source clear, descriptor image, tail, guard and post marker remain same-generation and disjoint",
        source_clear_and_memory_disjoint);

    TEST_END(ai_interrupt_memory_m_vmodule_mei_descriptor_doorbell_payload_ack_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_descriptor_ring_two_entry_clear_corner()
{
    TEST_START();

    const uintptr_t desc_base = phys_page_base(VSRWX_GRWX) + 0x800U;
    const uintptr_t tail_addr = phys_page_base(VSRWX_GRWX) + 0x880U;
    const uintptr_t ack_base = phys_page_base(VSRWX_GRWX) + 0x888U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0x8a0U;
    const uint64_t seq0 = 0x31ULL;
    const uint64_t seq1 = 0x32ULL;
    const uint64_t payload0 = 0x696d766472696e30ULL;
    const uint64_t payload1 = 0x696d766472696e31ULL;
    const uint64_t checksum0 = payload0 ^ (seq0 << 8U) ^
        0xa55aa55aa55aa55aULL;
    const uint64_t checksum1 = payload1 ^ (seq1 << 8U) ^
        0xa55aa55aa55aa55aULL;
    const uint64_t guard_seed = 0x696d766472696eaaULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    uint64_t rounds0 = 0ULL;
    uint64_t rounds1 = 0ULL;
    uint64_t mip_after_second = 0ULL;

    write64(desc_base + 0U, seq0);
    write64(desc_base + 8U, payload0);
    write64(desc_base + 16U, checksum0);
    write64(desc_base + 24U, 0ULL);
    write64(desc_base + 32U, seq1);
    write64(desc_base + 40U, payload1);
    write64(desc_base + 48U, checksum1);
    write64(desc_base + 56U, 0ULL);
    write64(tail_addr, 0ULL);
    write64(ack_base + 0U, 0ULL);
    write64(ack_base + 8U, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_reset_descriptor_log();
    ai_imv_desc_ring_base_addr = desc_base;
    ai_imv_desc_tail_addr = tail_addr;
    ai_imv_desc_ack_base_addr = ack_base;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_descriptor_ack_handler);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(4ULL);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();

    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    rounds0 = ai_imv_spin_until_desc_count(1ULL, AI_IMV_SPIN_LIMIT);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(10ULL);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_mei();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    rounds1 = ai_imv_spin_until_desc_count(2ULL, AI_IMV_SPIN_LIMIT);
    mip_after_second = CSRR(CSR_MIP);
    fence_iorw_iorw();

    const bool consumed_two_entries_in_order =
        rounds0 < AI_IMV_SPIN_LIMIT &&
        rounds1 < AI_IMV_SPIN_LIMIT &&
        ai_imv_trap_count == 2ULL &&
        ai_imv_trap_cause[0] == CAUSE_MEI &&
        ai_imv_trap_cause[1] == CAUSE_MEI &&
        ai_imv_desc_tail_log[0] == 0ULL &&
        ai_imv_desc_tail_log[1] == 1ULL &&
        ai_imv_desc_seq_log[0] == seq0 &&
        ai_imv_desc_seq_log[1] == seq1 &&
        ai_imv_desc_payload_log[0] == payload0 &&
        ai_imv_desc_payload_log[1] == payload1 &&
        ai_imv_desc_checksum_log[0] == checksum0 &&
        ai_imv_desc_checksum_log[1] == checksum1 &&
        ai_imv_desc_ack_log[0] == seq0 &&
        ai_imv_desc_ack_log[1] == seq1 &&
        read64(ack_base + 0U) == seq0 &&
        read64(ack_base + 8U) == seq1 &&
        read64(tail_addr) == 2ULL;
    const bool no_repeat_skip_or_source_leak =
        ai_imv_source_clear_log[0] == 1ULL &&
        ai_imv_source_clear_log[1] == 1ULL &&
        ai_imv_desc_seq_log[0] != ai_imv_desc_seq_log[1] &&
        read64(desc_base + 8U) == payload0 &&
        read64(desc_base + 40U) == payload1 &&
        read64(guard_addr) == guard_seed &&
        (mip_after_second & MIP_MEIP) == 0ULL;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_m_interrupt_state(saved_mstatus, saved_mie,
        saved_mideleg);
    ai_imv_desc_ring_base_addr = 0ULL;
    ai_imv_desc_tail_addr = 0ULL;
    ai_imv_desc_ack_base_addr = 0ULL;

    TEST_ASSERT("two delayed VModule MEI descriptor interrupts consume ring entries in sequence without repeat or skip",
        consumed_two_entries_in_order);
    TEST_ASSERT("each VModule source clear is exactly-once and does not alter descriptor payloads or guard",
        no_repeat_skip_or_source_leak);

    TEST_END(ai_interrupt_memory_m_vmodule_mei_descriptor_ring_two_entry_clear_corner);
}

bool ai_interrupt_memory_m_vmodule_mei_source_clear_interrupted_by_rnmi_owner_corner()
{
    TEST_START();

    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0xa00U;
    const uintptr_t ordinary_log_addr = phys_page_base(VSRWX_GRWX) + 0xa08U;
    const uintptr_t rnmi_log_addr = phys_page_base(VSRWX_GRWX) + 0xa10U;
    const uintptr_t post_addr = phys_page_base(VSRWX_GRWX) + 0xa18U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0xa20U;
    const uint64_t marker_before = 0x696d76776e646231ULL;
    const uint64_t marker_after = 0x696d76776e646232ULL;
    const uint64_t ordinary_log_seed = 0x696d76776f726400ULL;
    const uint64_t rnmi_log_seed = 0x696d7677726e6d00ULL;
    const uint64_t post_value = 0x696d7677706f7374ULL;
    const uint64_t guard_seed = 0x696d767767756172ULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    uint64_t ordinary_rounds = 0ULL;
    uint64_t final_mip = 0ULL;

    write64(marker_addr, marker_before);
    write64(ordinary_log_addr, ordinary_log_seed);
    write64(rnmi_log_addr, rnmi_log_seed);
    write64(post_addr, 0ULL);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_reset_window_log();
    ai_imv_window_marker_addr = marker_addr;
    ai_imv_window_ordinary_log_addr = ordinary_log_addr;
    ai_imv_window_rnmi_log_addr = rnmi_log_addr;
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_mei_clear_window_rnmi_handler);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_error_rnmi_mei_state();
    CSRS(CSR_MSTATUS, MSTATUS_MIE);

    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);
    ordinary_rounds = ai_imv_spin_until_window_ordinary(1ULL,
        AI_IMV_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    final_mip = CSRR(CSR_MIP);
    write64(marker_addr, marker_after);
    write64(post_addr, post_value);
    fence_iorw_iorw();

    for (uint64_t i = 0ULL; i < AI_IMV_SPIN_LIMIT; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }

    const bool ordinary_mei_clear_window_observed =
        ordinary_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_window_ordinary_count == 1ULL &&
        ai_imv_window_ordinary_cause[0] == CAUSE_MEI &&
        (ai_imv_window_ordinary_mip_before[0] & MIP_MEIP) != 0ULL &&
        (ai_imv_window_ordinary_mip_after_rnmi[0] & MIP_MEIP) != 0ULL &&
        (ai_imv_window_ordinary_mip_after_clear[0] & MIP_MEIP) == 0ULL &&
        ai_imv_window_ordinary_marker_before[0] == marker_before &&
        ai_imv_window_ordinary_marker_after_rnmi[0] == marker_before &&
        ai_imv_window_ordinary_clear_log[0] == 1ULL &&
        (final_mip & MIP_MEIP) == 0ULL;
    const bool rnmi_nested_in_clear_window_once =
        ai_imv_window_rnmi_count == 1ULL &&
        ai_imv_window_rnmi_mnret_markers == 1ULL &&
        ai_imv_window_rnmi_mncause[0] == AI_IMV_NMI31_CAUSE &&
        (ai_imv_window_rnmi_mnstatus[0] & MNSTATUS_NMIE) == 0ULL &&
        (ai_imv_window_rnmi_mip[0] & MIP_MEIP) != 0ULL &&
        ai_imv_window_rnmi_marker_value[0] == marker_before;
    const bool memory_logs_and_markers_remain_disjoint =
        read64(ordinary_log_addr) == 0x696d76776d656930ULL &&
        read64(rnmi_log_addr) == 0x696d76776e6d6931ULL &&
        read64(marker_addr) == marker_after &&
        read64(post_addr) == post_value &&
        read64(guard_addr) == guard_seed;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_error_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mnstatus);
    ai_imv_window_marker_addr = 0ULL;
    ai_imv_window_ordinary_log_addr = 0ULL;
    ai_imv_window_rnmi_log_addr = 0ULL;

    TEST_ASSERT("VModule MEI ordinary handler survives a nested NMI31 before source clear and clears only its own MEI source once",
        ordinary_mei_clear_window_observed,
        "rounds=%lx ordinary=%lx cause=%lx mip_before=%lx mip_after_rnmi=%lx mip_after_clear=%lx final_mip=%lx clear=%lx",
        ordinary_rounds, ai_imv_window_ordinary_count,
        ai_imv_window_ordinary_cause[0],
        ai_imv_window_ordinary_mip_before[0],
        ai_imv_window_ordinary_mip_after_rnmi[0],
        ai_imv_window_ordinary_mip_after_clear[0], final_mip,
        ai_imv_window_ordinary_clear_log[0]);
    TEST_ASSERT("the nested RNMI records exactly once, returns with mnret, and sees the ordinary MEI still pending before clear",
        rnmi_nested_in_clear_window_once,
        "rnmi=%lx mnret=%lx mncause=%lx mnstatus=%lx mip=%lx marker=%lx",
        ai_imv_window_rnmi_count, ai_imv_window_rnmi_mnret_markers,
        ai_imv_window_rnmi_mncause[0],
        ai_imv_window_rnmi_mnstatus[0],
        ai_imv_window_rnmi_mip[0],
        ai_imv_window_rnmi_marker_value[0]);
    TEST_ASSERT("ordinary handler log, RNMI log, mainline marker, post store and guard stay in separate memory owners",
        memory_logs_and_markers_remain_disjoint,
        "ordinary_log=%lx rnmi_log=%lx marker=%lx post=%lx guard=%lx",
        read64(ordinary_log_addr), read64(rnmi_log_addr),
        read64(marker_addr), read64(post_addr), read64(guard_addr));

    TEST_END(ai_interrupt_memory_m_vmodule_mei_source_clear_interrupted_by_rnmi_owner_corner);
}

bool ai_interrupt_memory_m_vmodule_rnmi_mnret_pending_mei_pmp_retry_corner()
{
    TEST_START();

    const enum test_page target_page = VSRWX_GRWX;
    const uintptr_t vaddr = hs_page_base(target_page) + 0xa80U;
    const uintptr_t paddr = phys_page_base(target_page) + 0xa80U;
    const uintptr_t marker_addr = phys_page_base(VSRWX_GRWX) + 0xa88U;
    const uintptr_t ordinary_log_addr = phys_page_base(VSRWX_GRWX) + 0xa90U;
    const uintptr_t rnmi_log_addr = phys_page_base(VSRWX_GRWX) + 0xa98U;
    const uintptr_t guard_addr = phys_page_base(VSRWX_GRWX) + 0xaa0U;
    const uintptr_t pmp_start = paddr & PAGE_ADDR_MSK;
    const uintptr_t pmp_end = pmp_start + PAGE_SIZE;
    const uint64_t target_seed = 0x696d767772747279ULL;
    const uint64_t marker_before = 0x696d76776d6e6231ULL;
    const uint64_t marker_after = 0x696d76776d6e6232ULL;
    const uint64_t ordinary_log_seed = 0x696d76776f7264f0ULL;
    const uint64_t rnmi_log_seed = 0x696d7677726e6df0ULL;
    const uint64_t guard_seed = 0x696d7677677561aaULL;
    const uint64_t load_sentinel = 0x696d7677726453eeULL;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_mnstatus = CSRR(CSR_MNSTATUS);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_pmpcfg0 = CSRR(CSR_PMPCFG0);
    const uint64_t saved_pmpaddr0 = CSRR(CSR_PMPADDR0);
    const uint64_t saved_pmpaddr1 = CSRR(CSR_PMPADDR1);
    const uint64_t saved_pmpaddr2 = CSRR(CSR_PMPADDR2);
    uint64_t fault_result = 0ULL;
    uint64_t fault_cause = 0ULL;
    uint64_t fault_tval = 0ULL;
    uint64_t pending_before_rnmi = 0ULL;
    uint64_t rnmi_rounds = 0ULL;
    uint64_t retry_result = 0ULL;
    uint64_t ordinary_rounds = 0ULL;
    uint64_t final_mip = 0ULL;

    write64(paddr, target_seed);
    write64(marker_addr, marker_before);
    write64(ordinary_log_addr, ordinary_log_seed);
    write64(rnmi_log_addr, rnmi_log_seed);
    write64(guard_addr, guard_seed);
    fence_iorw_iorw();

    ai_imv_reset_window_log();
    ai_imv_window_marker_addr = marker_addr;
    ai_imv_window_ordinary_log_addr = ordinary_log_addr;
    ai_imv_window_rnmi_log_addr = rnmi_log_addr;
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_mei_clear_window_rnmi_handler);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_error_rnmi_mei_state();
    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_MEI);

    for (uint64_t i = 0ULL; i < AI_IMV_SPIN_LIMIT &&
        (CSRR(CSR_MIP) & MIP_MEIP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    pending_before_rnmi = CSRR(CSR_MIP);

    ai_imv_restore_pmp_full_access();
    ai_imv_prepare_mprv_user_translation();
    ai_imv_configure_pmp_window(pmp_start, pmp_end, AI_IMV_PMP_TOR_NO_R);
    CSRW(CSR_MTVEC, (uintptr_t)mhandler_entry);
    ai_imv_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    fault_result = ai_imv_ld_probe(vaddr, load_sentinel);
    ai_imv_clear_mprv_translation();
    fault_cause = excpt.cause;
    fault_tval = excpt.tval;
    CSRW(CSR_MTVEC, (uintptr_t)ai_imv_mtvec_mei_clear_window_rnmi_handler);

    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_FORCE_NMI, 1ULL);
    rnmi_rounds = ai_imv_spin_until_window_rnmi(1ULL, AI_IMV_SPIN_LIMIT);

    ai_imv_configure_pmp_window(pmp_start, pmp_end, AI_IMV_PMP_TOR_RWX);
    ai_imv_enable_mprv_user_translation();
    TEST_SETUP_EXCEPT();
    retry_result = ai_imv_ld_probe(vaddr, load_sentinel);
    ai_imv_clear_mprv_translation();

    CSRS(CSR_MSTATUS, MSTATUS_MIE);
    ordinary_rounds = ai_imv_spin_until_window_ordinary(1ULL,
        AI_IMV_SPIN_LIMIT);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    final_mip = CSRR(CSR_MIP);
    write64(marker_addr, marker_after);
    fence_iorw_iorw();

    const bool pending_mei_survives_fault_and_rnmi =
        (pending_before_rnmi & MIP_MEIP) != 0ULL &&
        fault_result == load_sentinel &&
        fault_cause == CAUSE_LAF &&
        fault_tval == vaddr &&
        rnmi_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_window_rnmi_count == 1ULL &&
        ai_imv_window_rnmi_mnret_markers == 1ULL &&
        ai_imv_window_rnmi_mncause[0] == AI_IMV_NMI31_CAUSE &&
        (ai_imv_window_rnmi_mip[0] & MIP_MEIP) != 0ULL &&
        ai_imv_window_rnmi_marker_value[0] == marker_before;
    const bool memory_retry_uses_repaired_pmp_generation =
        retry_result == target_seed &&
        read64(paddr) == target_seed &&
        read64(guard_addr) == guard_seed;
    const bool ordinary_mei_after_mnret_is_consumed_once =
        ordinary_rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_window_ordinary_count == 1ULL &&
        ai_imv_window_ordinary_cause[0] == CAUSE_MEI &&
        ai_imv_window_ordinary_marker_before[0] == marker_before &&
        ai_imv_window_ordinary_marker_after_rnmi[0] == marker_before &&
        ai_imv_window_ordinary_clear_log[0] == 1ULL &&
        (ai_imv_window_ordinary_mip_after_clear[0] & MIP_MEIP) == 0ULL &&
        (final_mip & MIP_MEIP) == 0ULL &&
        read64(ordinary_log_addr) == 0x696d76776d656930ULL &&
        read64(rnmi_log_addr) == 0x696d76776e6d6931ULL &&
        read64(marker_addr) == marker_after;

    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_clear_error_sources();
    ai_imv_vmodule_set_delay(0ULL);
    ai_imv_clear_mprv_translation();
    ai_imv_restore_pmp_state(saved_pmpcfg0, saved_pmpaddr0,
        saved_pmpaddr1, saved_pmpaddr2);
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MTVEC, saved_mtvec);
    ai_imv_restore_error_state(saved_mstatus, saved_mie, saved_mideleg,
        saved_mnstatus);
    ai_imv_window_marker_addr = 0ULL;
    ai_imv_window_ordinary_log_addr = 0ULL;
    ai_imv_window_rnmi_log_addr = 0ULL;

    TEST_ASSERT("with VModule MEI pending, a PMP-denied load fault and intervening RNMI do not consume the ordinary MEI source",
        pending_mei_survives_fault_and_rnmi,
        "pending=%lx fault_result=%lx cause=%lx tval=%lx rnmi=%lx mnret=%lx rnmi_mip=%lx marker=%lx",
        pending_before_rnmi, fault_result, fault_cause, fault_tval,
        ai_imv_window_rnmi_count, ai_imv_window_rnmi_mnret_markers,
        ai_imv_window_rnmi_mip[0],
        ai_imv_window_rnmi_marker_value[0]);
    TEST_ASSERT("after mnret and PMP repair, the mainline retry load reads only the repaired target generation",
        memory_retry_uses_repaired_pmp_generation,
        "retry=%lx target=%lx guard=%lx",
        retry_result, read64(paddr), read64(guard_addr));
    TEST_ASSERT("after the repaired memory retry, the preserved ordinary VModule MEI is delivered and cleared exactly once",
        ordinary_mei_after_mnret_is_consumed_once,
        "ordinary=%lx cause=%lx mip_after_clear=%lx final_mip=%lx ordinary_log=%lx rnmi_log=%lx marker=%lx",
        ai_imv_window_ordinary_count,
        ai_imv_window_ordinary_cause[0],
        ai_imv_window_ordinary_mip_after_clear[0], final_mip,
        read64(ordinary_log_addr), read64(rnmi_log_addr),
        read64(marker_addr));

    TEST_END(ai_interrupt_memory_m_vmodule_rnmi_mnret_pending_mei_pmp_retry_corner);
}

bool ai_interrupt_memory_hs_vmodule_sei_clear_store_fault_retry_once_corner()
{
    TEST_START();

    const enum test_page marker_page = VSRWX_GRWX;
    const uintptr_t marker_paddr = phys_page_base(marker_page) + 0xbc0U;
    const uintptr_t marker_vaddr = hs_page_base(marker_page) + 0xbc0U;
    const uintptr_t log_paddr = phys_page_base(marker_page) + 0xbc8U;
    const uintptr_t log_vaddr = hs_page_base(marker_page) + 0xbc8U;
    const uint64_t marker_before = 0x696d7673636c6231ULL;
    const uint64_t marker_after = 0x696d7673636c6132ULL;
    const uint64_t log_seed = 0x696d7673636c6f67ULL;
    const pte_t no_write_mmio_leaf = PTE_V | PTE_AD | PTE_R;
    const pte_t write_mmio_leaf = PTE_V | PTE_AD | PTE_RW;
    const uint64_t saved_mstatus = CSRR(CSR_MSTATUS);
    const uint64_t saved_mie = CSRR(CSR_MIE);
    const uint64_t saved_mideleg = CSRR(CSR_MIDELEG);
    const uint64_t saved_medeleg = CSRR(CSR_MEDELEG);
    const uint64_t saved_mtvec = CSRR(CSR_MTVEC);
    const uint64_t saved_stvec = CSRR(CSR_STVEC);
    const uint64_t saved_satp = CSRR(CSR_SATP);
    const uint64_t saved_sstatus = CSRR(CSR_SSTATUS);
    const uint64_t saved_sie = CSRR(CSR_SIE);
    uint64_t rounds = 0ULL;
    uint64_t sip_before_enable = 0ULL;
    uint64_t sip_after_handler = 0ULL;
    pte_t repaired_pte_snapshot = 0ULL;

    goto_priv(PRIV_M);
    write64(marker_paddr, marker_before);
    write64(log_paddr, log_seed);
    fence_iorw_iorw();

    hspt_init();
    ai_imv_set_hs_leaf_to_paddr(marker_page, phys_page_base(marker_page),
        PTE_V | PTE_AD | PTE_RWX);
    ai_imv_map_vmodule_mmio_window(AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT,
        no_write_mmio_leaf);

    ai_imv_reset_clear_retry_log();
    ai_imv_clear_retry_marker_addr = marker_vaddr;
    ai_imv_clear_retry_log_addr = log_vaddr;
#ifdef sv48
    ai_imv_clear_retry_store_pte_addr =
        (uintptr_t)&ai_imv_vmodule_mmio_l3[
            PTE_INDEX(3, AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT)];
#else
    ai_imv_clear_retry_store_pte_addr =
        (uintptr_t)&ai_imv_vmodule_mmio_l3[
            PTE_INDEX(2, AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT)];
#endif
    ai_imv_clear_retry_store_pte_value =
        ((AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT & PAGE_ADDR_MSK) >> 2U) |
        write_mmio_leaf;

    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_STVEC, (uintptr_t)ai_imv_stvec_sei_clear_store_fault_retry_handler);
    CSRS(CSR_MEDELEG, 1ULL << CAUSE_SPF);
    TEST_SETUP_EXCEPT();
    ai_imv_prepare_vmodule_sei_in_hs();

    ai_imv_vmodule_write_reg(AI_IMV_VMODULE_REG_MAIN_INTERRUPT,
        AI_IMV_VMODULE_MAIN_SEI);
    for (uint64_t i = 0ULL; i < AI_IMV_SPIN_LIMIT &&
        (CSRR(CSR_SIP) & SIP_SEIP) == 0ULL; ++i) {
        asm volatile("addi x0, x0, 0" ::: "memory");
    }
    sip_before_enable = CSRR(CSR_SIP);

    goto_priv(PRIV_HS);
    rounds = ai_imv_spin_until_clear_retry_interrupt(1ULL, AI_IMV_SPIN_LIMIT);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    sip_after_handler = CSRR(CSR_SIP);
    write64(marker_vaddr, marker_after);
    fence_iorw_iorw();

    const bool source_remains_until_clear_store_retry =
        (sip_before_enable & SIP_SEIP) != 0ULL &&
        rounds < AI_IMV_SPIN_LIMIT &&
        ai_imv_clear_retry_interrupt_count == 1ULL &&
        ai_imv_clear_retry_fault_count == 1ULL &&
        ai_imv_clear_retry_clear_success_count == 1ULL &&
        ai_imv_clear_retry_scause[0] == CAUSE_SEI &&
        ai_imv_clear_retry_scause[1] == CAUSE_SPF &&
        ai_imv_clear_retry_stval[1] == AI_IMV_VMODULE_MMIO_MAIN_INTERRUPT &&
        (ai_imv_clear_retry_sip_before_clear[0] & SIP_SEIP) != 0ULL &&
        (ai_imv_clear_retry_sip_on_fault[1] & SIP_SEIP) != 0ULL &&
        ai_imv_clear_retry_faults_seen_at_clear == 1ULL;
    const bool source_clear_exactly_once_after_retry =
        (ai_imv_clear_retry_sip_after_clear[0] & SIP_SEIP) == 0ULL &&
        (sip_after_handler & SIP_SEIP) == 0ULL &&
        ai_imv_clear_retry_store_pte_addr != 0ULL &&
        *((volatile pte_t *)ai_imv_clear_retry_store_pte_addr) ==
            ai_imv_clear_retry_store_pte_value;
    const bool handler_memory_and_main_marker_disjoint =
        ai_imv_clear_retry_marker_before[0] == marker_before &&
        ai_imv_clear_retry_marker_after[0] == marker_before &&
        read64(log_paddr) == 0x696d7673636c5230ULL &&
        read64(marker_paddr) == marker_after;
    repaired_pte_snapshot = ai_imv_clear_retry_store_pte_addr == 0ULL ? 0ULL :
        *((volatile pte_t *)ai_imv_clear_retry_store_pte_addr);

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATUS, MSTATUS_MIE);
    CSRC(CSR_SSTATUS, SSTATUS_SIE_BIT);
    ai_imv_vmodule_clear_main_interrupt();
    ai_imv_vmodule_set_delay(0ULL);
    CSRW(CSR_STVEC, saved_stvec);
    CSRW(CSR_MTVEC, saved_mtvec);
    CSRW(CSR_MEDELEG, saved_medeleg);
    CSRW(CSR_MIDELEG, saved_mideleg);
    CSRW(CSR_MIE, saved_mie);
    CSRW(CSR_SIE, saved_sie);
    CSRW(CSR_SSTATUS, saved_sstatus);
    hspt_init();
    CSRW(CSR_SATP, saved_satp);
    sfence_vma();
    CSRW(CSR_MSTATUS, saved_mstatus);
    ai_imv_clear_retry_marker_addr = 0ULL;
    ai_imv_clear_retry_log_addr = 0ULL;
    ai_imv_clear_retry_store_pte_addr = 0ULL;
    ai_imv_clear_retry_store_pte_value = 0ULL;

    TEST_ASSERT("VModule SEI source stays pending while the handler clear-store faults, and the retry records the original clear address",
        source_remains_until_clear_store_retry,
        "sip_before=%lx rounds=%lx interrupts=%lx faults=%lx clears=%lx scause0=%lx scause1=%lx stval=%lx sip_fault=%lx faults_seen=%lx",
        sip_before_enable, rounds, ai_imv_clear_retry_interrupt_count,
        ai_imv_clear_retry_fault_count,
        ai_imv_clear_retry_clear_success_count,
        ai_imv_clear_retry_scause[0], ai_imv_clear_retry_scause[1],
        ai_imv_clear_retry_stval[1],
        ai_imv_clear_retry_sip_on_fault[1],
        ai_imv_clear_retry_faults_seen_at_clear);
    TEST_ASSERT("after the handler repairs the VModule MMIO clear leaf, the clear-store retries once and deasserts the level-sensitive source",
        source_clear_exactly_once_after_retry,
        "sip_after_clear=%lx final_sip=%lx pte_addr=%lx pte=%lx expected=%lx",
        ai_imv_clear_retry_sip_after_clear[0], sip_after_handler,
        ai_imv_clear_retry_store_pte_addr,
        repaired_pte_snapshot, ai_imv_clear_retry_store_pte_value);
    TEST_ASSERT("handler log and mainline marker remain separate across the clear-store page fault and retry",
        handler_memory_and_main_marker_disjoint,
        "marker_before=%lx marker_after_log=%lx log=%lx marker=%lx",
        ai_imv_clear_retry_marker_before[0],
        ai_imv_clear_retry_marker_after[0], read64(log_paddr),
        read64(marker_paddr));

    TEST_END(ai_interrupt_memory_hs_vmodule_sei_clear_store_fault_retry_once_corner);
}
