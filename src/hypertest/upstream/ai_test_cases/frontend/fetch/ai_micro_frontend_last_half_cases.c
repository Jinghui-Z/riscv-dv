#include <rvh_test.h>
#include <stdint.h>

static inline uint64_t ai_micro_frontend_last_half_split32_loop(uint64_t iterations)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option rvc\n\t"
        "mv t1, %[iterations]\n\t"
        "li t0, 0\n\t"
        "j 0f\n\t"
        ".balign 32, 0\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "addi t1, t1, -1\n\t"
        "bnez t1, 0b\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [iterations] "r"(iterations)
        : "t0", "t1", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_cross_32b_branch_refetch_no_lane_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t single = ai_micro_frontend_last_half_split32_loop(1);
    TEST_ASSERT("a split 32-bit addi at 32B+30 completes once without an exception or stale last-half state",
        excpt.triggered == false &&
        single == 1ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t triple = ai_micro_frontend_last_half_split32_loop(3);
    TEST_ASSERT("a taken branch refetching the same 32B last-half block three times keeps the split addi result exact",
        excpt.triggered == false &&
        triple == 3ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = ai_micro_frontend_last_half_split32_loop(17);
    TEST_ASSERT("repeated branch refetch of the 32B last-half split instruction does not drop the post-lane0 decrement or branch path",
        excpt.triggered == false &&
        repeated == 17ULL
    );

    TEST_END();
}

static inline uint64_t ai_micro_frontend_last_half_split_redirect_lane0_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option rvc\n\t"
        "li t0, 0\n\t"
        "j 0f\n\t"
        ".balign 32, 0\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "j 1f\n\t"
        ".balign 32, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 16\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_redirect_target_lane0_not_masked_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_redirect_lane0_probe();
    TEST_ASSERT("after a split 32B last-half RVI redirects to an unrelated aligned target, the target lane0 c.addi still executes",
        excpt.triggered == false &&
        observed == 17ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 8; ++i) {
        repeated += ai_micro_frontend_last_half_split_redirect_lane0_probe();
    }
    TEST_ASSERT("repeated split-lastHalf redirect probes do not leave stale valid_dup state that masks a later unrelated lane0 target",
        excpt.triggered == false &&
        repeated == 136ULL
    );

    TEST_END();
}

static inline uint64_t ai_micro_frontend_last_half_split_taken_branch_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option rvc\n\t"
        "li t0, 1\n\t"
        "j 0f\n\t"
        ".balign 32, 0\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 4\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_taken_branch_no_stale_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_taken_branch_probe();
    TEST_ASSERT("a taken conditional branch split at 32B+30 reaches only its target and does not retire stale fallthrough half state",
        excpt.triggered == false &&
        observed == 33ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 5; ++i) {
        repeated += ai_micro_frontend_last_half_split_taken_branch_probe();
    }
    TEST_ASSERT("repeated split taken-branch probes keep fallthrough skipped and leave no stale lastHalf state for later fetch packets",
        excpt.triggered == false &&
        repeated == 165ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_back_to_back_split_rvi_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "jal x0, 1f\n\t"
        ".rept 13\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "1:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0022\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_back_to_back_split_rvi_rearm_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_back_to_back_split_rvi_probe();
    TEST_ASSERT("two consecutive 32B+30 split RVI addi blocks both re-arm f3_lastHalf and retire without dropping the second split addi",
        excpt.triggered == false &&
        observed == 3ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 6; ++i) {
        repeated += ai_micro_frontend_last_half_back_to_back_split_rvi_probe();
    }
    TEST_ASSERT("repeated back-to-back split RVI probes keep f3_lastHalf clear/re-armed between split packets and preserve the exact accumulated result",
        excpt.triggered == false &&
        repeated == 18ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_following_rvi_lane1_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".4byte 0x00428293\n\t"
        ".4byte 0x00828293\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_following_rvi_lane1_not_dropped_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_following_rvi_lane1_probe();
    TEST_ASSERT("a split 32B+30 addi followed immediately by lane1/lane3 32-bit addi instructions preserves every post-high-half RVI lane",
        excpt.triggered == false &&
        observed == 13ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 9; ++i) {
        repeated += ai_micro_frontend_last_half_following_rvi_lane1_probe();
    }
    TEST_ASSERT("repeated lastHalf packets with immediate post-high-half RVI instructions keep f3_hasHalfValid/f3_lastHalf_mask from dropping lane1 work",
        excpt.triggered == false &&
        repeated == 117ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_split_jal_fixed_taken_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "jal t1, 1f\n\t"
        "2:\n\t"
        "addi t0, t0, 4\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "la t2, 2b\n\t"
        "sub t1, t1, t2\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_jal_fixed_taken_link_no_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_jal_fixed_taken_probe();
    TEST_ASSERT("a 32B+30 split JAL is fixed-taken, skips fallthrough, reaches the target, and writes the halfword-aligned PC+4 link",
        excpt.triggered == false &&
        observed == 97ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 7; ++i) {
        repeated += ai_micro_frontend_last_half_split_jal_fixed_taken_probe();
    }
    TEST_ASSERT("repeated split-JAL fixedTaken probes do not leave false lastHalf state, do not retire fallthrough poison, and keep link exact",
        excpt.triggered == false &&
        repeated == 679ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_split_jalr_indirect_target_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 1\n\t"
        "la t2, 1f\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "jalr t1, 0(t2)\n\t"
        "2:\n\t"
        "addi t0, t0, 4\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "la t3, 2b\n\t"
        "sub t1, t1, t3\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_jalr_indirect_target_link_no_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_jalr_indirect_target_probe();
    TEST_ASSERT("a 32B+30 split JALR uses the register target, skips fallthrough, and writes the halfword-aligned PC+4 link",
        excpt.triggered == false &&
        observed == 97ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 7; ++i) {
        repeated += ai_micro_frontend_last_half_split_jalr_indirect_target_probe();
    }
    TEST_ASSERT("repeated split-JALR probes keep indirect target and link exact without leaving stale lastHalf or executing fallthrough poison",
        excpt.triggered == false &&
        repeated == 679ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_split_jalr_odd_target_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 1\n\t"
        "la t2, 1f\n\t"
        "addi t2, t2, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 6\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "jalr t1, 0(t2)\n\t"
        "2:\n\t"
        "addi t0, t0, 4\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "la t3, 2b\n\t"
        "sub t1, t1, t3\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_jalr_odd_target_clear_bit_no_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_jalr_odd_target_probe();
    TEST_ASSERT("a 32B+30 split JALR with target bit0 set clears bit0, reaches the aligned target, skips fallthrough, and writes PC+4 link",
        excpt.triggered == false &&
        observed == 97ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 7; ++i) {
        repeated += ai_micro_frontend_last_half_split_jalr_odd_target_probe();
    }
    TEST_ASSERT("repeated split-JALR odd-target probes keep the bit0-cleared target and link exact without stale lastHalf fallthrough execution",
        excpt.triggered == false &&
        repeated == 679ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_split_branch_not_taken_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "bne x0, x0, 1f\n\t"
        "addi t0, t0, 4\n\t"
        "addi t0, t0, 8\n\t"
        "jal x0, 2f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 32\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_split_branch_not_taken_fallthrough_lanes_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_split_branch_not_taken_probe();
    TEST_ASSERT("a 32B+30 split conditional branch that is not taken consumes the high half and executes all fallthrough lanes without jumping to target",
        excpt.triggered == false &&
        observed == 13ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 11; ++i) {
        repeated += ai_micro_frontend_last_half_split_branch_not_taken_probe();
    }
    TEST_ASSERT("repeated split not-taken branch probes keep lastHalf fallthrough lanes valid and never retire the wrong target path",
        excpt.triggered == false &&
        repeated == 143ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_false_midblock_notcfi_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "addi t3, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".p2align 5, 0\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 3\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "beq x0, x0, 2f\n\t"
        "addi t0, t0, 8\n\t"
        "jal x0, 4f\n\t"
        ".p2align 5, 0\n\t"
        "2:\n\t"
        "bnez t3, 3f\n\t"
        "addi t3, x0, 1\n\t"
        "la t1, 1b\n\t"
        "lhu t4, 0(t1)\n\t"
        "lhu t5, 2(t1)\n\t"
        "li t2, 0x8293\n\t"
        "sh t2, 0(t1)\n\t"
        "li t2, 0x0052\n\t"
        "sh t2, 2(t1)\n\t"
        "fence.i\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0b\n\t"
        "3:\n\t"
        "addi t0, t0, 128\n\t"
        "4:\n\t"
        "la t1, 1b\n\t"
        "sh t4, 0(t1)\n\t"
        "sh t5, 2(t1)\n\t"
        "fence.i\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_false_midblock_notcfi_stale_ftq_seq_repair_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_false_midblock_notcfi_probe();
    TEST_ASSERT("a stale taken FTQ entry at a mid-block instruction rewritten to non-CFI addi must execute the addi and continue at the sequential half SNPC",
        excpt.triggered == false &&
        observed == 14ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 5; ++i) {
        repeated += ai_micro_frontend_last_half_false_midblock_notcfi_probe();
    }
    TEST_ASSERT("repeated branch-to-nonCFI rewrites keep false-lastHalf half-flush repair from jumping to the stale FTQ target or dropping the fallthrough addi",
        excpt.triggered == false &&
        repeated == 70ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_modulo_mixed_lanes_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.addi t0, 2\n\t"
        ".option norvc\n\t"
        ".4byte 0x00428293\n\t"
        ".option rvc\n\t"
        "c.addi t0, 8\n\t"
        ".option norvc\n\t"
        ".4byte 0x01028293\n\t"
        ".option rvc\n\t"
        "c.addi t0, 1\n\t"
        ".option norvc\n\t"
        ".4byte 0x02028293\n\t"
        ".option rvc\n\t"
        "c.addi t0, 3\n\t"
        ".option norvc\n\t"
        ".4byte 0x04028293\n\t"
        ".option rvc\n\t"
        "c.addi t0, 5\n\t"
        "c.addi t0, 6\n\t"
        ".2byte 0x0001\n\t"
        ".option norvc\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_modulo_mixed_rvc_rvi_lanes_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_modulo_mixed_lanes_probe();
    TEST_ASSERT("a lastHalf packet followed by mixed RVC and RVI work across lane4 lane8 and lane12 modulo groups retires every lane exactly once",
        excpt.triggered == false &&
        observed == 142ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 4; ++i) {
        repeated += ai_micro_frontend_last_half_modulo_mixed_lanes_probe();
    }
    TEST_ASSERT("repeated mixed-lane lastHalf packets keep valid_dup modulo replication and f3_hasHalfValid from dropping later lane groups",
        excpt.triggered == false &&
        repeated == 568ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_false_lane14_notcfi_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "addi t3, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 14\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        "1:\n\t"
        "beq x0, x0, 2f\n\t"
        "addi t0, t0, 8\n\t"
        "addi t0, t0, 16\n\t"
        "jal x0, 4f\n\t"
        ".p2align 5, 0\n\t"
        "2:\n\t"
        "bnez t3, 3f\n\t"
        "addi t3, x0, 1\n\t"
        "la t1, 1b\n\t"
        "lhu t4, 0(t1)\n\t"
        "lhu t5, 2(t1)\n\t"
        "li t2, 0x8293\n\t"
        "sh t2, 0(t1)\n\t"
        "li t2, 0x0052\n\t"
        "sh t2, 2(t1)\n\t"
        "fence.i\n\t"
        "addi t0, x0, 1\n\t"
        "jal x0, 0b\n\t"
        "3:\n\t"
        "addi t0, t0, 128\n\t"
        "4:\n\t"
        "la t1, 1b\n\t"
        "sh t4, 0(t1)\n\t"
        "sh t5, 2(t1)\n\t"
        "fence.i\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_false_lane14_notcfi_next_block_snpc_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_false_lane14_notcfi_probe();
    TEST_ASSERT("a false lastHalf at lane14 must redirect to the next 32B block start after a rewritten non-CFI instruction",
        excpt.triggered == false &&
        observed == 30ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 5; ++i) {
        repeated += ai_micro_frontend_last_half_false_lane14_notcfi_probe();
    }
    TEST_ASSERT("repeated lane14 false-lastHalf probes use the PredictWidth-2 f3_half_snpc path instead of the stale FTQ target",
        excpt.triggered == false &&
        repeated == 150ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_highhalf_cfi_shadow_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        ".4byte 0xa00902b7\n\t"
        "srli t0, t0, 16\n\t"
        "andi t0, t0, 15\n\t"
        "addi t0, t0, 2\n\t"
        "addi t0, t0, 4\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_masked_lane0_highhalf_cfi_no_false_taken_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_highhalf_cfi_shadow_probe();
    TEST_ASSERT("a split non-CFI LUI whose high half decodes like compressed CFI in lane0 must not create a false taken redirect",
        excpt.triggered == false &&
        observed == 15ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 6; ++i) {
        repeated += ai_micro_frontend_last_half_highhalf_cfi_shadow_probe();
    }
    TEST_ASSERT("repeated high-half-CFI-shadow split packets keep masked lane0 out of ftqOffset/fixedTaken while preserving lane1 sequential work",
        excpt.triggered == false &&
        repeated == 90ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_illegal_highhalf_shadow_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 3\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option norvc\n\t"
        ".4byte 0x00000013\n\t"
        "addi t0, t0, 7\n\t"
        "addi t0, t0, 9\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_masked_lane0_illegal_highhalf_no_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_illegal_highhalf_shadow_probe();
    TEST_ASSERT("a split 32-bit nop whose high half is 0x0000 in lane0 must not raise an illegal-instruction exception",
        excpt.triggered == false &&
        observed == 19ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t repeated = 0;
    for (unsigned i = 0; i < 8; ++i) {
        repeated += ai_micro_frontend_last_half_illegal_highhalf_shadow_probe();
    }
    TEST_ASSERT("repeated split-nop high-half-zero packets keep masked lane0 illegal shadow out of retirement while preserving sequential lane1 work",
        excpt.triggered == false &&
        repeated == 152ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_ebreak_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 5\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        ".4byte 0x00100073\n\t"
        "addi t0, t0, 8\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_ebreak_after_highhalf_precise_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_ebreak_probe();
    TEST_ASSERT("a real lane1 ebreak immediately after a split RVI high half must survive f3_lastHalf lane0 masking and report breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        observed == 14ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_ebreak_probe();
    TEST_ASSERT("a second lane1 ebreak probe after lastHalf consumption still reports precise breakpoint metadata and preserves sequential work after the trap",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        second == 14ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_illegal_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 6\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        ".4byte 0x0200200f\n\t"
        "addi t0, t0, 8\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_illegal_after_highhalf_precise_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_illegal_probe();
    TEST_ASSERT("a real lane1 illegal instruction after a split RVI high half must survive f3_lastHalf lane0 masking and report CAUSE_ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        observed == 15ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_illegal_probe();
    TEST_ASSERT("a second lane1 illegal probe after lastHalf consumption still preserves precise illegal metadata and resumes at the following lane",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        second == 15ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cebreak_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 7\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".2byte 0x9002\n\t"
        "c.addi t0, 8\n\t"
        ".option norvc\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cebreak_after_highhalf_precise_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cebreak_probe();
    TEST_ASSERT("a real lane1 c.ebreak after a split RVI high half must survive f3_lastHalf lane0 masking and report breakpoint",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        observed == 16ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cebreak_probe();
    TEST_ASSERT("a second lane1 c.ebreak probe still resumes by compressed length and preserves post-trap lane2 work",
        excpt.triggered == true &&
        excpt.cause == CAUSE_BKP &&
        second == 16ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cillegal_probe(void)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 8\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".2byte 0x0000\n\t"
        "c.addi t0, 8\n\t"
        ".option norvc\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        :
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cillegal_after_highhalf_precise_trap_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cillegal_probe();
    TEST_ASSERT("a real lane1 compressed-illegal 0x0000 after a split RVI high half must survive f3_lastHalf lane0 masking and report CAUSE_ILI",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        observed == 17ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cillegal_probe();
    TEST_ASSERT("a second lane1 compressed-illegal probe still resumes by compressed length and preserves post-trap lane2 work",
        excpt.triggered == true &&
        excpt.cause == CAUSE_ILI &&
        second == 17ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_store_probe(uint64_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 40\n\t"
        "mv t1, %[slot]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "sd t0, 0(t1)\n\t"
        "ld t2, 0(t1)\n\t"
        "addi t2, t2, 3\n\t"
        "mv %[observed], t2\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [slot] "r"(slot)
        : "t0", "t1", "t2", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_store_side_effect_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    uint64_t first_slot = 0x1111222233334444ULL;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_store_probe(&first_slot);
    TEST_ASSERT("a real lane1 store after a split RVI high half must survive f3_lastHalf lane0 masking and update memory",
        excpt.triggered == false &&
        first_slot == 41ULL &&
        observed == 44ULL
    );

    uint64_t second_slot = 0x5555666677778888ULL;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_store_probe(&second_slot);
    TEST_ASSERT("a second lane1 store probe keeps the store side effect and following load visible after lastHalf consumption",
        excpt.triggered == false &&
        second_slot == 41ULL &&
        second == 44ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_load_probe(const uint64_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "addi t0, x0, 10\n\t"
        "mv t1, %[slot]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "ld t2, 0(t1)\n\t"
        "add t0, t0, t2\n\t"
        "addi t0, t0, 3\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [slot] "r"(slot)
        : "t0", "t1", "t2", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_load_value_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    const uint64_t first_slot = 0x21ULL;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_load_probe(&first_slot);
    TEST_ASSERT("a real lane1 load after a split RVI high half must survive f3_lastHalf lane0 masking and feed the dependent add",
        excpt.triggered == false &&
        first_slot == 0x21ULL &&
        observed == 47ULL
    );

    const uint64_t second_slot = 0x2aULL;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_load_probe(&second_slot);
    TEST_ASSERT("a second lane1 load probe keeps the loaded value and dependent arithmetic visible after lastHalf consumption",
        excpt.triggered == false &&
        second_slot == 0x2aULL &&
        second == 56ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_taken_branch_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "beq x0, x0, 1f\n\t"
        "addi t0, t0, 64\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_taken_branch_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_taken_branch_probe(20);
    TEST_ASSERT("a real lane1 taken branch after a split RVI high half must survive f3_lastHalf lane0 masking and skip fallthrough work",
        excpt.triggered == false &&
        observed == 26ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_taken_branch_probe(3);
    TEST_ASSERT("a second lane1 taken branch probe keeps redirect metadata visible after lastHalf consumption",
        excpt.triggered == false &&
        second == 9ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_jal_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "jal t1, 1f\n\t"
        "2:\n\t"
        "addi t0, t0, 128\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "la t2, 2b\n\t"
        "sub t1, t1, t2\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "t1", "t2", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_jal_after_highhalf_link_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_jal_probe(4);
    TEST_ASSERT("a real lane1 JAL after a split RVI high half must survive f3_lastHalf lane0 masking, skip fallthrough poison, and write PC+4 link",
        excpt.triggered == false &&
        observed == 74ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_jal_probe(17);
    TEST_ASSERT("a second lane1 JAL probe preserves fixed-taken redirect and link metadata after lastHalf consumption",
        excpt.triggered == false &&
        second == 87ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_jalr_odd_target_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "la t2, 1f\n\t"
        "ori t2, t2, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 6\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".option norvc\n\t"
        "jalr t1, 0(t2)\n\t"
        "2:\n\t"
        "addi t0, t0, 128\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "la t3, 2b\n\t"
        "sub t1, t1, t3\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "t1", "t2", "t3", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_jalr_odd_target_after_highhalf_link_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_jalr_odd_target_probe(6);
    TEST_ASSERT("a real lane1 JALR with odd target after a split RVI high half must survive f3_lastHalf masking, clear bit0, skip fallthrough poison, and write PC+4 link",
        excpt.triggered == false &&
        observed == 76ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_jalr_odd_target_probe(23);
    TEST_ASSERT("a second lane1 JALR odd-target probe preserves indirect redirect, bit0 clear, and link metadata after lastHalf consumption",
        excpt.triggered == false &&
        second == 93ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cj_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.j 1f\n\t"
        ".option norvc\n\t"
        "addi t0, t0, 64\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cj_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cj_probe(30);
    TEST_ASSERT("a real lane1 compressed jump after a split RVI high half must survive f3_lastHalf lane0 masking and skip fallthrough poison",
        excpt.triggered == false &&
        observed == 36ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cj_probe(9);
    TEST_ASSERT("a second lane1 c.j probe preserves compressed fixed-taken redirect after lastHalf consumption",
        excpt.triggered == false &&
        second == 15ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cbeqz_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "addi a5, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.beqz a5, 1f\n\t"
        ".option norvc\n\t"
        "addi t0, t0, 64\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cbeqz_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cbeqz_probe(40);
    TEST_ASSERT("a real lane1 compressed beqz after a split RVI high half must survive f3_lastHalf lane0 masking and take the conditional redirect",
        excpt.triggered == false &&
        observed == 46ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cbeqz_probe(2);
    TEST_ASSERT("a second lane1 c.beqz probe preserves compressed conditional fixed-taken metadata after lastHalf consumption",
        excpt.triggered == false &&
        second == 8ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cbnez_not_taken_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "addi a5, x0, 0\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.bnez a5, 1f\n\t"
        ".option norvc\n\t"
        "addi t0, t0, 5\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cbnez_not_taken_after_highhalf_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cbnez_not_taken_probe(12);
    TEST_ASSERT("a real lane1 compressed bnez after a split RVI high half must survive f3_lastHalf lane0 masking and remain not-taken",
        excpt.triggered == false &&
        observed == 18ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cbnez_not_taken_probe(1);
    TEST_ASSERT("a second lane1 c.bnez not-taken probe preserves fallthrough valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        second == 7ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cbnez_taken_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "addi a5, x0, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.bnez a5, 1f\n\t"
        ".option norvc\n\t"
        "addi t0, t0, 64\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cbnez_taken_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cbnez_taken_probe(50);
    TEST_ASSERT("a real lane1 compressed bnez after a split RVI high half must survive f3_lastHalf lane0 masking and take the conditional redirect",
        excpt.triggered == false &&
        observed == 56ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cbnez_taken_probe(8);
    TEST_ASSERT("a second lane1 c.bnez taken probe preserves compressed fixed-taken metadata after lastHalf consumption",
        excpt.triggered == false &&
        second == 14ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cbeqz_not_taken_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "addi a5, x0, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.beqz a5, 1f\n\t"
        ".option norvc\n\t"
        "addi t0, t0, 5\n\t"
        "jal x0, 2f\n\t"
        "1:\n\t"
        "addi t0, t0, 64\n\t"
        "2:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cbeqz_not_taken_after_highhalf_fallthrough_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cbeqz_not_taken_probe(60);
    TEST_ASSERT("a real lane1 compressed beqz after a split RVI high half must survive f3_lastHalf lane0 masking and remain not-taken",
        excpt.triggered == false &&
        observed == 66ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cbeqz_not_taken_probe(7);
    TEST_ASSERT("a second lane1 c.beqz not-taken probe preserves fallthrough valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        second == 13ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cjr_odd_target_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "la t2, 1f\n\t"
        "ori t2, t2, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 6\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.jr t2\n\t"
        ".option norvc\n\t"
        "2:\n\t"
        "addi t0, t0, 128\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "3:\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "t2", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cjr_odd_target_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cjr_odd_target_probe(70);
    TEST_ASSERT("a real lane1 compressed jr with odd target after a split RVI high half must survive f3_lastHalf lane0 masking, clear bit0, and skip fallthrough poison",
        excpt.triggered == false &&
        observed == 76ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cjr_odd_target_probe(11);
    TEST_ASSERT("a second lane1 c.jr odd-target probe preserves compressed indirect redirect after lastHalf consumption",
        excpt.triggered == false &&
        second == 17ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cjalr_odd_target_link_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv t4, ra\n\t"
        "la t2, 1f\n\t"
        "ori t2, t2, 1\n\t"
        "jal x0, 0f\n\t"
        ".rept 4\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.jalr t2\n\t"
        ".option norvc\n\t"
        "2:\n\t"
        "addi t0, t0, 128\n\t"
        "jal x0, 3f\n\t"
        ".p2align 5, 0\n\t"
        "1:\n\t"
        "addi t0, t0, 5\n\t"
        "la t3, 2b\n\t"
        "sub t1, ra, t3\n\t"
        "sltiu t1, t1, 1\n\t"
        "slli t1, t1, 6\n\t"
        "add t0, t0, t1\n\t"
        "3:\n\t"
        "mv ra, t4\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "t1", "t2", "t3", "t4", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cjalr_odd_target_link_after_highhalf_redirect_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cjalr_odd_target_link_probe(9);
    TEST_ASSERT("a real lane1 compressed jalr with odd target after a split RVI high half must survive f3_lastHalf masking, clear bit0, skip fallthrough poison, and write PC+2 link",
        excpt.triggered == false &&
        observed == 79ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cjalr_odd_target_link_probe(24);
    TEST_ASSERT("a second lane1 c.jalr odd-target probe preserves compressed indirect redirect and PC+2 link after lastHalf consumption",
        excpt.triggered == false &&
        second == 94ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_csd_store_probe(uint64_t seed, volatile uint64_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a4, %[slot]\n\t"
        "addi a5, t0, 17\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.sd a5, 0(a4)\n\t"
        ".option norvc\n\t"
        "ld t1, 0(a4)\n\t"
        "add t0, t0, t1\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [slot] "r"(slot)
        : "t0", "t1", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_csd_store_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    volatile uint64_t first_slot = 0x1111222233334444ULL;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_csd_store_probe(21, &first_slot);
    TEST_ASSERT("a real lane1 compressed sd after a split RVI high half must survive f3_lastHalf lane0 masking and commit its store side effect",
        excpt.triggered == false &&
        observed == 60ULL &&
        first_slot == 38ULL
    );

    volatile uint64_t second_slot = 0x5555666677778888ULL;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_csd_store_probe(5, &second_slot);
    TEST_ASSERT("a second lane1 c.sd probe preserves compressed store valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        second == 28ULL &&
        second_slot == 22ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cld_load_probe(uint64_t seed, volatile uint64_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a4, %[slot]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.ld a5, 0(a4)\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [slot] "r"(slot)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cld_load_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    volatile uint64_t first_slot = 29ULL;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cld_load_probe(13, &first_slot);
    TEST_ASSERT("a real lane1 compressed ld after a split RVI high half must survive f3_lastHalf lane0 masking and feed its loaded value to the dependent add",
        excpt.triggered == false &&
        observed == 43ULL &&
        first_slot == 29ULL
    );

    volatile uint64_t second_slot = 50ULL;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cld_load_probe(4, &second_slot);
    TEST_ASSERT("a second lane1 c.ld probe preserves compressed load valid/enqEnable and data dependency after lastHalf consumption",
        excpt.triggered == false &&
        second == 55ULL &&
        second_slot == 50ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_clw_signext_probe(uint64_t seed, volatile uint32_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a4, %[slot]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.lw a5, 0(a4)\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [slot] "r"(slot)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_clw_signext_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    volatile uint32_t negative_word = 0xfffffff0U;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_clw_signext_probe(24, &negative_word);
    TEST_ASSERT("a real lane1 compressed lw after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend its 32-bit load value",
        excpt.triggered == false &&
        observed == 9ULL &&
        negative_word == 0xfffffff0U
    );

    volatile uint32_t positive_word = 31U;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_clw_signext_probe(6, &positive_word);
    TEST_ASSERT("a second lane1 c.lw probe preserves compressed word-load valid/enqEnable and the dependent add dataflow after lastHalf consumption",
        excpt.triggered == false &&
        second == 38ULL &&
        positive_word == 31U
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_csw_word_store_probe(uint64_t seed, volatile uint64_t *slot)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a4, %[slot]\n\t"
        "addi a5, t0, 17\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.sw a5, 0(a4)\n\t"
        ".option norvc\n\t"
        "lwu t1, 0(a4)\n\t"
        "add t0, t0, t1\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [slot] "r"(slot)
        : "t0", "t1", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_csw_word_store_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    volatile uint64_t first_slot = 0xaabbccdd55556666ULL;
    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_csw_word_store_probe(21, &first_slot);
    TEST_ASSERT("a real lane1 compressed sw after a split RVI high half must survive f3_lastHalf lane0 masking and update only the low 32-bit word",
        excpt.triggered == false &&
        observed == 60ULL &&
        first_slot == 0xaabbccdd00000026ULL
    );

    volatile uint64_t second_slot = 0x11223344deadbeefULL;
    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_csw_word_store_probe(5, &second_slot);
    TEST_ASSERT("a second lane1 c.sw probe preserves compressed word-store valid/enqEnable and leaves the adjacent upper word intact",
        excpt.triggered == false &&
        second == 28ULL &&
        second_slot == 0x1122334400000016ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_caddiw_signext_probe(uint64_t seed)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "jal x0, 0f\n\t"
        ".rept 12\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.addiw t0, 1\n\t"
        ".option norvc\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed)
        : "t0", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_caddiw_signext_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_caddiw_signext_probe(0x000000007ffffffeULL);
    TEST_ASSERT("a real lane1 compressed addiw after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend the 32-bit ALU result",
        excpt.triggered == false &&
        observed == 0xffffffff80000000ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_caddiw_signext_probe(40ULL);
    TEST_ASSERT("a second lane1 c.addiw probe preserves compressed ALU valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 42ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cxor_regreg_probe(uint64_t seed, uint64_t lhs, uint64_t rhs)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[lhs]\n\t"
        "mv a4, %[rhs]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.xor a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cxor_regreg_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cxor_regreg_probe(3, 0x55, 0xaa);
    TEST_ASSERT("a real lane1 compressed xor after a split RVI high half must survive f3_lastHalf lane0 masking and consume both compressed-register sources",
        excpt.triggered == false &&
        observed == 259ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cxor_regreg_probe(0x10, 0xf0, 0x0f);
    TEST_ASSERT("a second lane1 c.xor probe preserves CA-format register-register ALU valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        second == 272ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_csubw_signext_probe(uint64_t seed, uint64_t lhs, uint64_t rhs)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[lhs]\n\t"
        "mv a4, %[rhs]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.subw a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_csubw_signext_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_csubw_signext_probe(5, 0x000000007fffffffULL, 0x00000000ffffffffULL);
    TEST_ASSERT("a real lane1 compressed subw after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend the 32-bit register-register result",
        excpt.triggered == false &&
        observed == 0xffffffff80000006ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_csubw_signext_probe(9, 0x40, 0x11);
    TEST_ASSERT("a second lane1 c.subw probe preserves CA-format word ALU valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 57ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_csrai_shift_probe(uint64_t seed, uint64_t value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.srai a5, 2\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [value] "r"(value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_csrai_shift_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_csrai_shift_probe(9, 0xfffffffffffffff0ULL);
    TEST_ASSERT("a real lane1 compressed srai after a split RVI high half must survive f3_lastHalf lane0 masking and preserve arithmetic sign bits",
        excpt.triggered == false &&
        observed == 6ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_csrai_shift_probe(4, 0x40ULL);
    TEST_ASSERT("a second lane1 c.srai probe preserves compressed immediate-shift valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 21ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_candi_imm_probe(uint64_t seed, uint64_t value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.andi a5, -16\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [value] "r"(value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_candi_imm_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_candi_imm_probe(9, 0xffffffffffffffffULL);
    TEST_ASSERT("a real lane1 compressed andi after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend its immediate",
        excpt.triggered == false &&
        observed == 0xfffffffffffffffaULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_candi_imm_probe(5, 0x1234ULL);
    TEST_ASSERT("a second lane1 c.andi probe preserves compressed immediate ALU valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 4662ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cor_regreg_probe(uint64_t seed, uint64_t lhs, uint64_t rhs)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[lhs]\n\t"
        "mv a4, %[rhs]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.or a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cor_regreg_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cor_regreg_probe(6, 0x00ff00ff00000000ULL, 0xf00000000000000fULL);
    TEST_ASSERT("a real lane1 compressed or after a split RVI high half must survive f3_lastHalf lane0 masking and preserve the register-register OR dataflow",
        excpt.triggered == false &&
        observed == 0xf0ff00ff00000016ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_cor_regreg_probe(3, 0x1230ULL, 0x00cfULL);
    TEST_ASSERT("a second lane1 c.or probe preserves CA-format source-register valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 0x1303ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cand_regreg_probe(uint64_t seed, uint64_t lhs, uint64_t rhs)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[lhs]\n\t"
        "mv a4, %[rhs]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.and a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cand_regreg_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cand_regreg_probe(6, 0xf0ff00ff0000000fULL, 0x0fff00ff0000ffffULL);
    TEST_ASSERT("a real lane1 compressed and after a split RVI high half must survive f3_lastHalf lane0 masking and preserve the register-register AND dataflow",
        excpt.triggered == false &&
        observed == 0x00ff00ff00000016ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_cand_regreg_probe(3, 0x12f3ULL, 0x0f0fULL);
    TEST_ASSERT("a second lane1 c.and probe preserves CA-format source-register valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 0x207ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_csrli_shift_probe(uint64_t seed, uint64_t value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.srli a5, 2\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [value] "r"(value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_csrli_shift_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_csrli_shift_probe(9, 0xfffffffffffffff0ULL);
    TEST_ASSERT("a real lane1 compressed srli after a split RVI high half must survive f3_lastHalf lane0 masking and zero-fill the logical right shift result",
        excpt.triggered == false &&
        observed == 0x4000000000000006ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_csrli_shift_probe(7, 0x80ULL);
    TEST_ASSERT("a second lane1 c.srli probe preserves compressed immediate-shift valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 40ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cslli_shift_probe(uint64_t seed, uint64_t value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.slli a5, 3\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [value] "r"(value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cslli_shift_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cslli_shift_probe(9, 0x1000000000000001ULL);
    TEST_ASSERT("a real lane1 compressed slli after a split RVI high half must survive f3_lastHalf lane0 masking and preserve the left-shift dataflow",
        excpt.triggered == false &&
        observed == 0x8000000000000012ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t positive = ai_micro_frontend_last_half_lane1_cslli_shift_probe(7, 0x12ULL);
    TEST_ASSERT("a second lane1 c.slli probe preserves compressed immediate left-shift valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        positive == 152ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cli_imm_probe(uint64_t seed, uint64_t old_value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[old_value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.li a5, -1\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [old_value] "r"(old_value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cli_imm_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cli_imm_probe(9, 0x1234ULL);
    TEST_ASSERT("a real lane1 compressed li after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend its CI-format immediate",
        excpt.triggered == false &&
        observed == 9ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cli_imm_probe(41, 0xffffULL);
    TEST_ASSERT("a second lane1 c.li probe overwrites the old destination value after lastHalf consumption",
        excpt.triggered == false &&
        second == 41ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cadd_regreg_probe(uint64_t seed, uint64_t lhs, uint64_t rhs)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[lhs]\n\t"
        "mv a4, %[rhs]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.add a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [lhs] "r"(lhs), [rhs] "r"(rhs)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cadd_regreg_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cadd_regreg_probe(6, 0xfffffffffffffff0ULL, 0x20ULL);
    TEST_ASSERT("a real lane1 compressed add after a split RVI high half must survive f3_lastHalf lane0 masking and preserve the full-XLEN register-register add dataflow",
        excpt.triggered == false &&
        observed == 0x17ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cadd_regreg_probe(3, 0x1234000000000000ULL, 0x00000000000000ffULL);
    TEST_ASSERT("a second lane1 c.add probe preserves CR-format source-register valid/enqEnable after lastHalf consumption",
        excpt.triggered == false &&
        second == 0x1234000000000103ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_cmv_reg_probe(uint64_t seed, uint64_t old_value, uint64_t source)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[old_value]\n\t"
        "mv a4, %[source]\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.mv a5, a4\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [old_value] "r"(old_value), [source] "r"(source)
        : "t0", "a4", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_cmv_reg_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_cmv_reg_probe(6, 0xffffffffffffffffULL, 0x123456789abcdef0ULL);
    TEST_ASSERT("a real lane1 compressed mv after a split RVI high half must survive f3_lastHalf lane0 masking and copy the CR-format source register",
        excpt.triggered == false &&
        observed == 0x123456789abcdef7ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_cmv_reg_probe(3, 0x7777ULL, 0x40ULL);
    TEST_ASSERT("a second lane1 c.mv probe overwrites the old destination value after lastHalf consumption",
        excpt.triggered == false &&
        second == 68ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_clui_imm_probe(uint64_t seed, uint64_t old_value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[old_value]\n\t"
        "jal x0, 0f\n\t"
        ".rept 10\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        ".2byte 0x7781\n\t"
        ".option norvc\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [old_value] "r"(old_value)
        : "t0", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_clui_imm_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_clui_imm_probe(6, 0x12345678ULL);
    TEST_ASSERT("a real lane1 compressed lui after a split RVI high half must survive f3_lastHalf lane0 masking and sign-extend its upper immediate write",
        excpt.triggered == false &&
        observed == 0xfffffffffffe0007ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_clui_imm_probe(3, 0xffffffffffffffffULL);
    TEST_ASSERT("a second lane1 c.lui probe overwrites the old destination value after lastHalf consumption",
        excpt.triggered == false &&
        second == 0xfffffffffffe0004ULL
    );

    TEST_END();
}

__attribute__((noinline, aligned(32))) static uint64_t ai_micro_frontend_last_half_lane1_caddi4spn_sp_probe(uint64_t seed, uint64_t old_value)
{
    uint64_t observed;

    asm volatile(
        ".option push\n\t"
        ".option norelax\n\t"
        ".option norvc\n\t"
        "mv t0, %[seed]\n\t"
        "mv a5, %[old_value]\n\t"
        "mv t1, sp\n\t"
        "jal x0, 0f\n\t"
        ".rept 8\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".option rvc\n\t"
        "0:\n\t"
        ".rept 15\n\t"
        ".2byte 0x0001\n\t"
        ".endr\n\t"
        ".2byte 0x8293\n\t"
        ".2byte 0x0012\n\t"
        "c.addi4spn a5, sp, 64\n\t"
        ".option norvc\n\t"
        "sub a5, a5, t1\n\t"
        "add t0, t0, a5\n\t"
        "mv %[observed], t0\n\t"
        ".option pop\n\t"
        : [observed] "=r"(observed)
        : [seed] "r"(seed), [old_value] "r"(old_value)
        : "t0", "t1", "a5", "memory");

    return observed;
}

bool ai_micro_frontend_last_half_lane1_caddi4spn_sp_after_highhalf_no_drop_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    uint64_t observed = ai_micro_frontend_last_half_lane1_caddi4spn_sp_probe(6, 0xffffffffffffffffULL);
    TEST_ASSERT("a real lane1 compressed addi4spn after a split RVI high half must survive f3_lastHalf lane0 masking and write sp-plus-immediate into the compressed destination",
        excpt.triggered == false &&
        observed == 71ULL
    );

    TEST_SETUP_EXCEPT();
    uint64_t second = ai_micro_frontend_last_half_lane1_caddi4spn_sp_probe(3, 0x12345678ULL);
    TEST_ASSERT("a second lane1 c.addi4spn probe overwrites the old compressed destination value after lastHalf consumption",
        excpt.triggered == false &&
        second == 68ULL
    );

    TEST_END();
}
