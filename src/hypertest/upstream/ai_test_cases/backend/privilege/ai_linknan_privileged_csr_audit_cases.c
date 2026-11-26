#include <rvh_test.h>
#include <stdint.h>

#define AI_LINKNAN_STR2(x) #x
#define AI_LINKNAN_STR(x) AI_LINKNAN_STR2(x)

#define AI_LINKNAN_CSRR_NUM(csr) \
    asm volatile("csrr t0, " AI_LINKNAN_STR(csr) ::: "t0", "memory")

#define AI_LINKNAN_CSRW_NUM(csr, value) do { \
    uint64_t __ai_linknan_csrw_value = (uint64_t)(value); \
    asm volatile("csrw " AI_LINKNAN_STR(csr) ", %0" \
        :: "rK"(__ai_linknan_csrw_value) : "memory"); \
} while (0)

#define AI_LINKNAN_READ_CSR_NUM(csr, out) \
    asm volatile("csrr %0, " AI_LINKNAN_STR(csr) : "=r"(out) :: "memory")

#define AI_LINKNAN_EXPECT_READ_ILI(label, csr) do { \
    TEST_SETUP_EXCEPT(); \
    AI_LINKNAN_CSRR_NUM(csr); \
    TEST_ASSERT(label " read traps as illegal instruction", \
        excpt.triggered == true && excpt.cause == CAUSE_ILI); \
} while (0)

#define AI_LINKNAN_EXPECT_WRITE_ILI(label, csr) do { \
    TEST_SETUP_EXCEPT(); \
    AI_LINKNAN_CSRW_NUM(csr, 0x5a5a5a5a5a5a5a5aULL); \
    TEST_ASSERT(label " write traps as illegal instruction", \
        excpt.triggered == true && excpt.cause == CAUSE_ILI); \
} while (0)

#define AI_LINKNAN_ASSERT_NO_EXCEPT(label) \
    TEST_ASSERT(label, excpt.triggered == false)

#define AI_LINKNAN_CSR_SSP             0x011
#define AI_LINKNAN_CSR_SEED            0x015
#define AI_LINKNAN_CSR_JVT             0x017
#define AI_LINKNAN_CSR_SCOUNTINHIBIT   0x120
#define AI_LINKNAN_CSR_SRM_CFG         0x181
#define AI_LINKNAN_CSR_SBPCTL          0x5c0
#define AI_LINKNAN_CSR_SPFCTL          0x5c1
#define AI_LINKNAN_CSR_SMBLOCKCTL      0x5c3
#define AI_LINKNAN_CSR_SRNCTL          0x5c4
#define AI_LINKNAN_CSR_SCONTEXT_SPEC   0x5a8
#define AI_LINKNAN_CSR_HCONTEXT_SPEC   0x6a8
#define AI_LINKNAN_CSR_MCONTEXT_SPEC   0x7a8
#define AI_LINKNAN_CSR_SIREG2          0x152
#define AI_LINKNAN_CSR_SIREG3          0x153
#define AI_LINKNAN_CSR_SIREG4          0x155
#define AI_LINKNAN_CSR_SIREG5          0x156
#define AI_LINKNAN_CSR_SIREG6          0x157
#define AI_LINKNAN_CSR_VSIREG2         0x252
#define AI_LINKNAN_CSR_VSIREG3         0x253
#define AI_LINKNAN_CSR_VSIREG4         0x255
#define AI_LINKNAN_CSR_VSIREG5         0x256
#define AI_LINKNAN_CSR_VSIREG6         0x257
#define AI_LINKNAN_CSR_MVIP            0x309
#define AI_LINKNAN_CSR_MCYCLECFG       0x321
#define AI_LINKNAN_CSR_MINSTRETCFG     0x322
#define AI_LINKNAN_CSR_MCTRCTL         0x34e
#define AI_LINKNAN_CSR_MISELECT        0x350
#define AI_LINKNAN_CSR_MIREG           0x351
#define AI_LINKNAN_CSR_MIREG2          0x352
#define AI_LINKNAN_CSR_MIREG3          0x353
#define AI_LINKNAN_CSR_MIREG4          0x355
#define AI_LINKNAN_CSR_MIREG5          0x356
#define AI_LINKNAN_CSR_MIREG6          0x357
#define AI_LINKNAN_CSR_MTOPEI          0x35c
#define AI_LINKNAN_CSR_MTOPI           0xfb0
#define AI_LINKNAN_CSR_HVIEN_SPEC      0x608
#define AI_LINKNAN_CSR_HVICTL_SPEC     0x609
#define AI_LINKNAN_CSR_HVIPRIO1        0x646
#define AI_LINKNAN_CSR_HVIPRIO2        0x647
#define AI_LINKNAN_CSR_SCTRCTL         0x14e
#define AI_LINKNAN_CSR_SCTRSTATUS      0x14f
#define AI_LINKNAN_CSR_SCTRDEPTH       0x15f
#define AI_LINKNAN_CSR_VSCTRCTL        0x24e

#define AI_LINKNAN_ENVCFG_RO0_MASK \
    ((1ULL << 61) | (3ULL << 32) | (1ULL << 3) | \
     (1ULL << 2) | (1ULL << 0))
#define AI_LINKNAN_ENVCFG_CBO_MASK \
    ((1ULL << 7) | (1ULL << 6) | (3ULL << 4))
#define AI_LINKNAN_SENVCFG_RO0_MASK \
    ((1ULL << 63) | (1ULL << 62) | AI_LINKNAN_ENVCFG_RO0_MASK)
#define AI_LINKNAN_MSTATEEN0_RO0_MASK \
    ((1ULL << 60) | (1ULL << 59) | (1ULL << 58) | \
     (1ULL << 57) | (1ULL << 56))
#define AI_LINKNAN_SSTATEEN0_RO0_MASK ((1ULL << 2) | (1ULL << 1))
#define AI_LINKNAN_MHPMEVENT_VS_VU_INH_MASK ((1ULL << 59) | (1ULL << 58))
#define AI_LINKNAN_MSECCFG_RO0_MASK \
    ((1ULL << 10) | (1ULL << 9) | (1ULL << 8) | \
     (1ULL << 2) | (1ULL << 1) | (1ULL << 0))

static inline uint64_t ai_linknan_read_backend_custom_csr(
    unsigned csr, uint64_t sentinel)
{
    uint64_t value = sentinel;

    switch (csr) {
    case AI_LINKNAN_CSR_SBPCTL:
        asm volatile("csrr %0, 0x5c0" : "+r"(value) :: "memory");
        break;
    case AI_LINKNAN_CSR_SPFCTL:
        asm volatile("csrr %0, 0x5c1" : "+r"(value) :: "memory");
        break;
    case AI_LINKNAN_CSR_SMBLOCKCTL:
        asm volatile("csrr %0, 0x5c3" : "+r"(value) :: "memory");
        break;
    case AI_LINKNAN_CSR_SRNCTL:
        asm volatile("csrr %0, 0x5c4" : "+r"(value) :: "memory");
        break;
    default:
        break;
    }

    return value;
}

static inline void ai_linknan_write_backend_custom_csr(
    unsigned csr, uint64_t value)
{
    switch (csr) {
    case AI_LINKNAN_CSR_SBPCTL:
        asm volatile("csrw 0x5c0, %0" :: "rK"(value) : "memory");
        break;
    case AI_LINKNAN_CSR_SPFCTL:
        asm volatile("csrw 0x5c1, %0" :: "rK"(value) : "memory");
        break;
    case AI_LINKNAN_CSR_SMBLOCKCTL:
        asm volatile("csrw 0x5c3, %0" :: "rK"(value) : "memory");
        break;
    case AI_LINKNAN_CSR_SRNCTL:
        asm volatile("csrw 0x5c4, %0" :: "rK"(value) : "memory");
        break;
    default:
        break;
    }
}

bool ai_linknan_priv_aia_extension_csr_write_illegal_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mvien CSR with EnableAIA disabled", CSR_MVIEN);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mvip CSR with EnableAIA disabled", AI_LINKNAN_CSR_MVIP);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine miselect CSR with EnableAIA disabled", AI_LINKNAN_CSR_MISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg2 CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG2);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg3 CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG3);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg4 CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG4);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg5 CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG5);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mireg6 CSR with EnableAIA disabled", AI_LINKNAN_CSR_MIREG6);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine mtopei CSR with EnableAIA disabled", AI_LINKNAN_CSR_MTOPEI);

    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor siselect CSR with EnableAIA disabled", CSR_SISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg CSR with EnableAIA disabled", CSR_SIREG);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg2 CSR with EnableAIA disabled", AI_LINKNAN_CSR_SIREG2);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg3 CSR with EnableAIA disabled", AI_LINKNAN_CSR_SIREG3);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg4 CSR with EnableAIA disabled", AI_LINKNAN_CSR_SIREG4);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg5 CSR with EnableAIA disabled", AI_LINKNAN_CSR_SIREG5);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor sireg6 CSR with EnableAIA disabled", AI_LINKNAN_CSR_SIREG6);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor stopei CSR with EnableAIA disabled", CSR_STOPEI);

    AI_LINKNAN_EXPECT_WRITE_ILI("AIA hypervisor hvien CSR with EnableAIA disabled", AI_LINKNAN_CSR_HVIEN_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA hypervisor hvictl CSR with EnableAIA disabled", AI_LINKNAN_CSR_HVICTL_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA hypervisor hviprio1 CSR with EnableAIA disabled", AI_LINKNAN_CSR_HVIPRIO1);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA hypervisor hviprio2 CSR with EnableAIA disabled", AI_LINKNAN_CSR_HVIPRIO2);

    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsiselect CSR with EnableAIA disabled", CSR_VSISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg CSR with EnableAIA disabled", CSR_VSIREG);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg2 CSR with EnableAIA disabled", AI_LINKNAN_CSR_VSIREG2);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg3 CSR with EnableAIA disabled", AI_LINKNAN_CSR_VSIREG3);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg4 CSR with EnableAIA disabled", AI_LINKNAN_CSR_VSIREG4);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg5 CSR with EnableAIA disabled", AI_LINKNAN_CSR_VSIREG5);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vsireg6 CSR with EnableAIA disabled", AI_LINKNAN_CSR_VSIREG6);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor vstopei CSR with EnableAIA disabled", CSR_VSTOPEI);

    // TOPI writes are illegal even when AIA is implemented because TOPI is read-only.
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA machine read-only mtopi CSR", AI_LINKNAN_CSR_MTOPI);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA supervisor read-only stopi CSR", CSR_STOPI);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA virtual-supervisor read-only vstopi CSR", CSR_VSTOPI);

    TEST_ASSERT("all AIA-added CSR write probes report illegal instruction when LinkNan EnableAIA is false",
        test_status == true);

    TEST_END(ai_linknan_priv_aia_extension_csr_write_illegal_matrix_corner);
}

bool ai_linknan_priv_mvip_hvip_write_illegal_without_aia_h_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    const uint64_t misa_value = CSRR(CSR_MISA);
    AI_LINKNAN_ASSERT_NO_EXCEPT(
        "M-mode can read misa before probing mvip and hvip writes");
    TEST_ASSERT("LinkNan current profile keeps misa.H read-only zero",
        (misa_value & MISA_H) == 0ULL);

    AI_LINKNAN_EXPECT_WRITE_ILI(
        "AIA machine mvip CSR with EnableAIA disabled",
        AI_LINKNAN_CSR_MVIP);
    AI_LINKNAN_EXPECT_WRITE_ILI(
        "H hvip CSR when the LinkNan profile does not expose H",
        CSR_HVIP);

    TEST_ASSERT(
        "LinkNan rejects both the AIA mvip write and the unavailable H hvip write",
        test_status == true);

    TEST_END(ai_linknan_priv_mvip_hvip_write_illegal_without_aia_h_corner);
}

bool ai_linknan_priv_unsupported_machine_csr_illegal_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    AI_LINKNAN_EXPECT_READ_ILI("unsupported unprivileged Zicfiss ssp CSR", AI_LINKNAN_CSR_SSP);
    AI_LINKNAN_EXPECT_WRITE_ILI("unsupported unprivileged Zicfiss ssp CSR", AI_LINKNAN_CSR_SSP);
    AI_LINKNAN_EXPECT_READ_ILI("unsupported Zkr seed CSR", AI_LINKNAN_CSR_SEED);
    AI_LINKNAN_EXPECT_WRITE_ILI("unsupported Zkr seed CSR", AI_LINKNAN_CSR_SEED);
    AI_LINKNAN_EXPECT_READ_ILI("unsupported Zcmt jvt CSR", AI_LINKNAN_CSR_JVT);
    AI_LINKNAN_EXPECT_WRITE_ILI("unsupported Zcmt jvt CSR", AI_LINKNAN_CSR_JVT);

    AI_LINKNAN_EXPECT_READ_ILI("machine indirect miselect CSR with AIA/CSRIND disabled", AI_LINKNAN_CSR_MISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("machine indirect miselect CSR with AIA/CSRIND disabled", AI_LINKNAN_CSR_MISELECT);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg CSR with AIA/CSRIND disabled", AI_LINKNAN_CSR_MIREG);
    AI_LINKNAN_EXPECT_WRITE_ILI("machine indirect mireg CSR with AIA/CSRIND disabled", AI_LINKNAN_CSR_MIREG);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg2 CSR", AI_LINKNAN_CSR_MIREG2);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg3 CSR", AI_LINKNAN_CSR_MIREG3);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg4 CSR", AI_LINKNAN_CSR_MIREG4);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg5 CSR", AI_LINKNAN_CSR_MIREG5);
    AI_LINKNAN_EXPECT_READ_ILI("machine indirect mireg6 CSR", AI_LINKNAN_CSR_MIREG6);

    AI_LINKNAN_EXPECT_READ_ILI("AIA mvien CSR when EnableAIA is false", CSR_MVIEN);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA mvien CSR when EnableAIA is false", CSR_MVIEN);
    AI_LINKNAN_EXPECT_READ_ILI("AIA mtopei CSR when EnableAIA is false", AI_LINKNAN_CSR_MTOPEI);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA mtopei CSR when EnableAIA is false", AI_LINKNAN_CSR_MTOPEI);

    AI_LINKNAN_EXPECT_READ_ILI("Smcntrpmf mcyclecfg CSR", AI_LINKNAN_CSR_MCYCLECFG);
    AI_LINKNAN_EXPECT_WRITE_ILI("Smcntrpmf mcyclecfg CSR", AI_LINKNAN_CSR_MCYCLECFG);
    AI_LINKNAN_EXPECT_READ_ILI("Smcntrpmf minstretcfg CSR", AI_LINKNAN_CSR_MINSTRETCFG);
    AI_LINKNAN_EXPECT_WRITE_ILI("Smcntrpmf minstretcfg CSR", AI_LINKNAN_CSR_MINSTRETCFG);
    AI_LINKNAN_EXPECT_READ_ILI("Smctr mctrctl CSR", AI_LINKNAN_CSR_MCTRCTL);
    AI_LINKNAN_EXPECT_WRITE_ILI("Smctr mctrctl CSR", AI_LINKNAN_CSR_MCTRCTL);

    AI_LINKNAN_EXPECT_READ_ILI("debug trigger tdata3 CSR", CSR_TDATA3);
    AI_LINKNAN_EXPECT_WRITE_ILI("debug trigger tdata3 CSR", CSR_TDATA3);
    AI_LINKNAN_EXPECT_READ_ILI("Sdtrig mcontext CSR", AI_LINKNAN_CSR_MCONTEXT_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("Sdtrig mcontext CSR", AI_LINKNAN_CSR_MCONTEXT_SPEC);

    AI_LINKNAN_EXPECT_READ_ILI("machine mvip CSR module is forced illegal", AI_LINKNAN_CSR_MVIP);
    AI_LINKNAN_EXPECT_WRITE_ILI("machine mvip CSR module is forced illegal", AI_LINKNAN_CSR_MVIP);
    AI_LINKNAN_EXPECT_READ_ILI("machine mtinst CSR module is forced illegal", CSR_MTINST);
    AI_LINKNAN_EXPECT_WRITE_ILI("machine mtinst CSR module is forced illegal", CSR_MTINST);
    AI_LINKNAN_EXPECT_READ_ILI("machine mtval2 CSR module is forced illegal", CSR_MTVAL2);
    AI_LINKNAN_EXPECT_WRITE_ILI("machine mtval2 CSR module is forced illegal", CSR_MTVAL2);

    TEST_ASSERT("all audited unsupported/forced-illegal machine CSR probes report illegal instruction",
        test_status == true);

    TEST_END(ai_linknan_priv_unsupported_machine_csr_illegal_matrix_corner);
}

bool ai_linknan_priv_unsupported_supervisor_csr_illegal_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    AI_LINKNAN_EXPECT_READ_ILI("supervisor scountinhibit CSR", AI_LINKNAN_CSR_SCOUNTINHIBIT);
    AI_LINKNAN_EXPECT_WRITE_ILI("supervisor scountinhibit CSR", AI_LINKNAN_CSR_SCOUNTINHIBIT);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor srmcfg CSR", AI_LINKNAN_CSR_SRM_CFG);
    AI_LINKNAN_EXPECT_WRITE_ILI("supervisor srmcfg CSR", AI_LINKNAN_CSR_SRM_CFG);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor scontext CSR at privileged spec address", AI_LINKNAN_CSR_SCONTEXT_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("supervisor scontext CSR at privileged spec address", AI_LINKNAN_CSR_SCONTEXT_SPEC);

    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect siselect CSR with AIA/CSRIND disabled", CSR_SISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("supervisor indirect siselect CSR with AIA/CSRIND disabled", CSR_SISELECT);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg CSR with AIA/CSRIND disabled", CSR_SIREG);
    AI_LINKNAN_EXPECT_WRITE_ILI("supervisor indirect sireg CSR with AIA/CSRIND disabled", CSR_SIREG);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg2 CSR", AI_LINKNAN_CSR_SIREG2);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg3 CSR", AI_LINKNAN_CSR_SIREG3);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg4 CSR", AI_LINKNAN_CSR_SIREG4);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg5 CSR", AI_LINKNAN_CSR_SIREG5);
    AI_LINKNAN_EXPECT_READ_ILI("supervisor indirect sireg6 CSR", AI_LINKNAN_CSR_SIREG6);

    AI_LINKNAN_EXPECT_READ_ILI("AIA stopei CSR when EnableAIA is false", CSR_STOPEI);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA stopei CSR when EnableAIA is false", CSR_STOPEI);
    AI_LINKNAN_EXPECT_READ_ILI("AIA stopi CSR when EnableAIA is false", CSR_STOPI);
    AI_LINKNAN_EXPECT_WRITE_ILI("AIA stopi CSR when EnableAIA is false", CSR_STOPI);

    AI_LINKNAN_EXPECT_READ_ILI("Ssctr sctrctl CSR", AI_LINKNAN_CSR_SCTRCTL);
    AI_LINKNAN_EXPECT_WRITE_ILI("Ssctr sctrctl CSR", AI_LINKNAN_CSR_SCTRCTL);
    AI_LINKNAN_EXPECT_READ_ILI("Ssctr sctrstatus CSR", AI_LINKNAN_CSR_SCTRSTATUS);
    AI_LINKNAN_EXPECT_WRITE_ILI("Ssctr sctrstatus CSR", AI_LINKNAN_CSR_SCTRSTATUS);
    AI_LINKNAN_EXPECT_READ_ILI("Ssctr sctrdepth CSR", AI_LINKNAN_CSR_SCTRDEPTH);
    AI_LINKNAN_EXPECT_WRITE_ILI("Ssctr sctrdepth CSR", AI_LINKNAN_CSR_SCTRDEPTH);

    TEST_ASSERT("all audited unsupported supervisor CSR probes report illegal instruction",
        test_status == true);

    TEST_END(ai_linknan_priv_unsupported_supervisor_csr_illegal_matrix_corner);
}

bool ai_linknan_priv_h_vs_csr_illegal_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);

    TEST_SETUP_EXCEPT();
    const uint64_t misa_value = CSRR(CSR_MISA);
    AI_LINKNAN_ASSERT_NO_EXCEPT("M-mode can read misa before checking H extension state");
    TEST_ASSERT("LinkNan current profile keeps misa.H read-only zero",
        (misa_value & MISA_H) == 0ULL);

    AI_LINKNAN_EXPECT_READ_ILI("H hstatus CSR when H extension is disabled", CSR_HSTATUS);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hstatus CSR when H extension is disabled", CSR_HSTATUS);
    AI_LINKNAN_EXPECT_READ_ILI("H hedeleg CSR when H extension is disabled", CSR_HEDELEG);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hedeleg CSR when H extension is disabled", CSR_HEDELEG);
    AI_LINKNAN_EXPECT_READ_ILI("H hideleg CSR when H extension is disabled", CSR_HIDELEG);
    AI_LINKNAN_EXPECT_READ_ILI("H hie CSR when H extension is disabled", CSR_HIE);
    AI_LINKNAN_EXPECT_READ_ILI("H hcounteren CSR when H extension is disabled", CSR_HCOUNTEREN);
    AI_LINKNAN_EXPECT_READ_ILI("H henvcfg CSR when H extension is disabled", CSR_HENVCFG);
    AI_LINKNAN_EXPECT_WRITE_ILI("H henvcfg CSR when H extension is disabled", CSR_HENVCFG);
    AI_LINKNAN_EXPECT_READ_ILI("H hgatp CSR when H extension is disabled", CSR_HGATP);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hgatp CSR when H extension is disabled", CSR_HGATP);
    AI_LINKNAN_EXPECT_READ_ILI("H hstateen0 CSR when H extension is disabled", CSR_HSTATEEN0);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hstateen0 CSR when H extension is disabled", CSR_HSTATEEN0);
    AI_LINKNAN_EXPECT_READ_ILI("H hvictl CSR when AIA/H are disabled", AI_LINKNAN_CSR_HVICTL_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hvictl CSR when AIA/H are disabled", AI_LINKNAN_CSR_HVICTL_SPEC);
    AI_LINKNAN_EXPECT_READ_ILI("H hviprio1 CSR when AIA/H are disabled", AI_LINKNAN_CSR_HVIPRIO1);
    AI_LINKNAN_EXPECT_READ_ILI("H hviprio2 CSR when AIA/H are disabled", AI_LINKNAN_CSR_HVIPRIO2);
    AI_LINKNAN_EXPECT_READ_ILI("H hcontext CSR at privileged spec address", AI_LINKNAN_CSR_HCONTEXT_SPEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("H hcontext CSR at privileged spec address", AI_LINKNAN_CSR_HCONTEXT_SPEC);

    AI_LINKNAN_EXPECT_READ_ILI("VS vsstatus CSR when H extension is disabled", CSR_VSSTATUS);
    AI_LINKNAN_EXPECT_WRITE_ILI("VS vsstatus CSR when H extension is disabled", CSR_VSSTATUS);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsie CSR when H extension is disabled", CSR_VSIE);
    AI_LINKNAN_EXPECT_READ_ILI("VS vstvec CSR when H extension is disabled", CSR_VSTVEC);
    AI_LINKNAN_EXPECT_WRITE_ILI("VS vstvec CSR when H extension is disabled", CSR_VSTVEC);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsscratch CSR when H extension is disabled", CSR_VSSCRATCH);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsepc CSR when H extension is disabled", CSR_VSEPC);
    AI_LINKNAN_EXPECT_READ_ILI("VS vscause CSR when H extension is disabled", CSR_VSCAUSE);
    AI_LINKNAN_EXPECT_READ_ILI("VS vstval CSR when H extension is disabled", CSR_VSTVAL);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsip CSR when H extension is disabled", CSR_VSIP);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsatp CSR when H extension is disabled", CSR_VSATP);
    AI_LINKNAN_EXPECT_WRITE_ILI("VS vsatp CSR when H extension is disabled", CSR_VSATP);
    AI_LINKNAN_EXPECT_READ_ILI("VS vstimecmp CSR when H extension is disabled", CSR_VSTIMECMP);
    AI_LINKNAN_EXPECT_WRITE_ILI("VS vstimecmp CSR when H extension is disabled", CSR_VSTIMECMP);

    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsiselect CSR when H/AIA are disabled", CSR_VSISELECT);
    AI_LINKNAN_EXPECT_WRITE_ILI("VS indirect vsiselect CSR when H/AIA are disabled", CSR_VSISELECT);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg CSR when H/AIA are disabled", CSR_VSIREG);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg2 CSR", AI_LINKNAN_CSR_VSIREG2);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg3 CSR", AI_LINKNAN_CSR_VSIREG3);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg4 CSR", AI_LINKNAN_CSR_VSIREG4);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg5 CSR", AI_LINKNAN_CSR_VSIREG5);
    AI_LINKNAN_EXPECT_READ_ILI("VS indirect vsireg6 CSR", AI_LINKNAN_CSR_VSIREG6);
    AI_LINKNAN_EXPECT_READ_ILI("VS vstopei CSR when H/AIA are disabled", CSR_VSTOPEI);
    AI_LINKNAN_EXPECT_READ_ILI("VS vstopi CSR when H/AIA are disabled", CSR_VSTOPI);
    AI_LINKNAN_EXPECT_READ_ILI("VS vsctrctl CSR", AI_LINKNAN_CSR_VSCTRCTL);

    TEST_END(ai_linknan_priv_h_vs_csr_illegal_matrix_corner);
}

bool ai_linknan_priv_envcfg_stateen_ro0_warl_matrix_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_menvcfg = CSRR(CSR_MENVCFG);
    const uint64_t saved_senvcfg = CSRR(CSR_SENVCFG);
    const uint64_t saved_mstateen0 = CSRR(CSR_MSTATEEN0);
    uint64_t saved_mstateen1 = 0ULL;
    uint64_t saved_mstateen2 = 0ULL;
    uint64_t saved_mstateen3 = 0ULL;
    const uint64_t saved_sstateen0 = CSRR(CSR_SSTATEEN0);
    uint64_t saved_sstateen1 = 0ULL;
    uint64_t saved_sstateen2 = 0ULL;
    uint64_t saved_sstateen3 = 0ULL;
    const uint64_t saved_mhpmevent3 = CSRR(CSR_MHPMEVENT3);
    const uint64_t saved_mconfigptr = CSRR(CSR_MCONFIGPTR);
    const uint64_t menvcfg_write =
        saved_menvcfg | AI_LINKNAN_ENVCFG_RO0_MASK | AI_LINKNAN_ENVCFG_CBO_MASK |
        (1ULL << 63) | (1ULL << 62);
    const uint64_t senvcfg_write =
        saved_senvcfg | AI_LINKNAN_SENVCFG_RO0_MASK | AI_LINKNAN_ENVCFG_CBO_MASK;

    AI_LINKNAN_READ_CSR_NUM(0x30d, saved_mstateen1);
    AI_LINKNAN_READ_CSR_NUM(0x30e, saved_mstateen2);
    AI_LINKNAN_READ_CSR_NUM(0x30f, saved_mstateen3);
    AI_LINKNAN_READ_CSR_NUM(0x10d, saved_sstateen1);
    AI_LINKNAN_READ_CSR_NUM(0x10e, saved_sstateen2);
    AI_LINKNAN_READ_CSR_NUM(0x10f, saved_sstateen3);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MENVCFG, menvcfg_write);
    const uint64_t menvcfg_after = CSRR(CSR_MENVCFG);
    AI_LINKNAN_ASSERT_NO_EXCEPT("M-mode MENVCFG field probe does not trap");
    TEST_ASSERT("MENVCFG unsupported ADUE/PMM/SSE/LPE/FIOM fields remain read-only zero",
        (menvcfg_after & AI_LINKNAN_ENVCFG_RO0_MASK) == 0ULL);
    TEST_ASSERT("MENVCFG implemented STCE/PBMTE/CBO fields can be observed as CSR state",
        (menvcfg_after & ((1ULL << 63) | (1ULL << 62))) ==
            (menvcfg_write & ((1ULL << 63) | (1ULL << 62))) &&
        (menvcfg_after & AI_LINKNAN_ENVCFG_CBO_MASK) ==
            (menvcfg_write & AI_LINKNAN_ENVCFG_CBO_MASK));

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SENVCFG, senvcfg_write);
    const uint64_t senvcfg_after = CSRR(CSR_SENVCFG);
    AI_LINKNAN_ASSERT_NO_EXCEPT("M-mode SENVCFG field probe does not trap");
    TEST_ASSERT("SENVCFG STCE/PBMTE and unsupported extension fields remain read-only zero",
        (senvcfg_after & AI_LINKNAN_SENVCFG_RO0_MASK) == 0ULL);
    TEST_ASSERT("SENVCFG CBO fields follow the base EnvCfg implemented field behavior",
        (senvcfg_after & AI_LINKNAN_ENVCFG_CBO_MASK) ==
            (senvcfg_write & AI_LINKNAN_ENVCFG_CBO_MASK));

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MSTATEEN0, ~0ULL);
    const uint64_t mstateen0_after = CSRR(CSR_MSTATEEN0);
    AI_LINKNAN_ASSERT_NO_EXCEPT("MSTATEEN0 all-ones field probe does not trap");
    TEST_ASSERT("MSTATEEN0 CSRIND/AIA/IMSIC/CONTEXT/P1P13 remain read-only zero",
        (mstateen0_after & AI_LINKNAN_MSTATEEN0_RO0_MASK) == 0ULL);
    TEST_ASSERT("MSTATEEN0 implemented SE0/ENVCFG/C bits can be set",
        (mstateen0_after & (MSTATEEN_SE0 | MSTATEEN_ENVCFG | MSTATEEN_C)) ==
            (MSTATEEN_SE0 | MSTATEEN_ENVCFG | MSTATEEN_C));

    TEST_SETUP_EXCEPT();
    CSRW(CSR_SSTATEEN0, ~0ULL);
    const uint64_t sstateen0_after = CSRR(CSR_SSTATEEN0);
    AI_LINKNAN_ASSERT_NO_EXCEPT("SSTATEEN0 all-ones field probe does not trap");
    TEST_ASSERT("SSTATEEN0 JVT/FCSR fields remain read-only zero while C can be set",
        (sstateen0_after & AI_LINKNAN_SSTATEEN0_RO0_MASK) == 0ULL &&
        (sstateen0_after & MSTATEEN_C) == MSTATEEN_C);

    TEST_SETUP_EXCEPT();
    AI_LINKNAN_CSRW_NUM(0x30d, ~0ULL);
    uint64_t mstateen1_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x30d, mstateen1_after);
    AI_LINKNAN_CSRW_NUM(0x30e, ~0ULL);
    uint64_t mstateen2_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x30e, mstateen2_after);
    AI_LINKNAN_CSRW_NUM(0x30f, ~0ULL);
    uint64_t mstateen3_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x30f, mstateen3_after);
    AI_LINKNAN_CSRW_NUM(0x10d, ~0ULL);
    uint64_t sstateen1_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x10d, sstateen1_after);
    AI_LINKNAN_CSRW_NUM(0x10e, ~0ULL);
    uint64_t sstateen2_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x10e, sstateen2_after);
    AI_LINKNAN_CSRW_NUM(0x10f, ~0ULL);
    uint64_t sstateen3_after = 0ULL;
    AI_LINKNAN_READ_CSR_NUM(0x10f, sstateen3_after);
    AI_LINKNAN_ASSERT_NO_EXCEPT("stateen1-3 probes complete without trapping");
    TEST_ASSERT("MSTATEEN1-3 only expose implemented SE bits when written all ones",
        (mstateen1_after & ~(1ULL << 63)) == 0ULL &&
        (mstateen2_after & ~(1ULL << 63)) == 0ULL &&
        (mstateen3_after & ~(1ULL << 63)) == 0ULL);
    TEST_ASSERT("SSTATEEN1-3 are implemented as read-only zero",
        sstateen1_after == 0ULL &&
        sstateen2_after == 0ULL &&
        sstateen3_after == 0ULL);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MHPMEVENT3, ~0ULL);
    const uint64_t mhpmevent3_after = CSRR(CSR_MHPMEVENT3);
    AI_LINKNAN_ASSERT_NO_EXCEPT("MHPMEVENT3 all-ones field probe does not trap");
    TEST_ASSERT("MHPMEVENT3 VSINH/VUINH remain read-only zero",
        (mhpmevent3_after & AI_LINKNAN_MHPMEVENT_VS_VU_INH_MASK) == 0ULL);

    TEST_SETUP_EXCEPT();
    const uint64_t mconfigptr_after_read = CSRR(CSR_MCONFIGPTR);
    AI_LINKNAN_ASSERT_NO_EXCEPT("MCONFIGPTR read is legal");
    TEST_ASSERT("MCONFIGPTR reads as the implemented read-only zero value",
        mconfigptr_after_read == 0ULL);

    TEST_SETUP_EXCEPT();
    AI_LINKNAN_CSRW_NUM(CSR_MCONFIGPTR, saved_mconfigptr ^ 0x40ULL);
    TEST_ASSERT("MCONFIGPTR write traps because the CSR is read-only",
        excpt.triggered == true && excpt.cause == CAUSE_ILI);
    TEST_ASSERT("MCONFIGPTR value remains unchanged after illegal write",
        CSRR(CSR_MCONFIGPTR) == saved_mconfigptr);

    AI_LINKNAN_CSRW_NUM(0x30d, saved_mstateen1);
    AI_LINKNAN_CSRW_NUM(0x30e, saved_mstateen2);
    AI_LINKNAN_CSRW_NUM(0x30f, saved_mstateen3);
    CSRW(CSR_SSTATEEN0, saved_sstateen0);
    AI_LINKNAN_CSRW_NUM(0x10d, saved_sstateen1);
    AI_LINKNAN_CSRW_NUM(0x10e, saved_sstateen2);
    AI_LINKNAN_CSRW_NUM(0x10f, saved_sstateen3);
    CSRW(CSR_MSTATEEN0, saved_mstateen0);
    CSRW(CSR_MHPMEVENT3, saved_mhpmevent3);
    CSRW(CSR_SENVCFG, saved_senvcfg);
    CSRW(CSR_MENVCFG, saved_menvcfg);

    TEST_END(ai_linknan_priv_envcfg_stateen_ro0_warl_matrix_corner);
}

bool ai_linknan_priv_mseccfg_ro0_current_rtl_observation_corner()
{
    TEST_START();

    goto_priv(PRIV_M);
    const uint64_t saved_mseccfg = CSRR(CSR_MSECCFG);

    TEST_SETUP_EXCEPT();
    CSRW(CSR_MSECCFG, saved_mseccfg | AI_LINKNAN_MSECCFG_RO0_MASK);
    const uint64_t mseccfg_after = CSRR(CSR_MSECCFG);
    AI_LINKNAN_ASSERT_NO_EXCEPT("LinkNan current RTL exposes MSECCFG as an accessible CSR");
    TEST_ASSERT("MSECCFG unsupported extension bits MLPE/SSEED/USEED/RLB/MMWP/MML read back zero",
        (mseccfg_after & AI_LINKNAN_MSECCFG_RO0_MASK) == 0ULL);

    CSRW(CSR_MSECCFG, saved_mseccfg);

    TEST_END(ai_linknan_priv_mseccfg_ro0_current_rtl_observation_corner);
}

bool ai_linknan_priv_mstateen0_c_controls_smode_custom_csr_rw_corner()
{
    TEST_START();

    static const unsigned custom_csrs[] = {
        AI_LINKNAN_CSR_SBPCTL,
        AI_LINKNAN_CSR_SPFCTL,
        AI_LINKNAN_CSR_SMBLOCKCTL,
        AI_LINKNAN_CSR_SRNCTL,
    };
    static const uint64_t writable_probe_masks[] = {
        1ULL,
        1ULL,
        1ULL,
        1ULL,
    };
    uint64_t saved_custom_csrs[sizeof(custom_csrs) / sizeof(custom_csrs[0])];
    uint64_t opened_custom_csrs[sizeof(custom_csrs) / sizeof(custom_csrs[0])];

    goto_priv(PRIV_M);
    const uint64_t saved_mstateen0 = CSRR(CSR_MSTATEEN0);

    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        saved_custom_csrs[i] =
            ai_linknan_read_backend_custom_csr(custom_csrs[i], 0ULL);
        TEST_ASSERT("M-mode can save each implemented backend custom CSR before the state-enable matrix",
            excpt.triggered == false,
            "csr=%x cause=%lx", custom_csrs[i], excpt.cause);
    }

    CSRS(CSR_MSTATEEN0, MSTATEEN_C);
    TEST_ASSERT("mstateen0.C can be set before entering S-mode",
        (CSRR(CSR_MSTATEEN0) & MSTATEEN_C) != 0ULL);

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        const uint64_t opened_read =
            ai_linknan_read_backend_custom_csr(custom_csrs[i], 0ULL);
        TEST_ASSERT("S-mode custom CSR read is legal when mstateen0.C is set",
            excpt.triggered == false,
            "csr=%x cause=%lx", custom_csrs[i], excpt.cause);

        const uint64_t probe_value = opened_read ^ writable_probe_masks[i];
        TEST_SETUP_EXCEPT();
        ai_linknan_write_backend_custom_csr(custom_csrs[i], probe_value);
        TEST_ASSERT("S-mode custom CSR write is legal when mstateen0.C is set",
            excpt.triggered == false,
            "csr=%x cause=%lx", custom_csrs[i], excpt.cause);

        TEST_SETUP_EXCEPT();
        opened_custom_csrs[i] =
            ai_linknan_read_backend_custom_csr(custom_csrs[i], 0ULL);
        TEST_ASSERT("S-mode custom CSR write updates the selected writable bit when mstateen0.C is set",
            excpt.triggered == false &&
            (opened_custom_csrs[i] & writable_probe_masks[i]) ==
                (probe_value & writable_probe_masks[i]),
            "csr=%x observed=%lx expected=%lx",
            custom_csrs[i], opened_custom_csrs[i], probe_value);
    }

    goto_priv(PRIV_M);
    CSRC(CSR_MSTATEEN0, MSTATEEN_C);
    TEST_ASSERT("mstateen0.C can be cleared before the denied S-mode probes",
        (CSRR(CSR_MSTATEEN0) & MSTATEEN_C) == 0ULL);

    goto_priv(PRIV_HS);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); ++i) {
        const uint64_t read_sentinel =
            0x5a5a000000000000ULL | (uint64_t)custom_csrs[i];

        TEST_SETUP_EXCEPT();
        const uint64_t blocked_read =
            ai_linknan_read_backend_custom_csr(custom_csrs[i], read_sentinel);
        TEST_ASSERT("S-mode custom CSR read traps as illegal instruction when mstateen0.C is clear",
            excpt.triggered == true &&
            excpt.cause == CAUSE_ILI &&
            blocked_read == read_sentinel,
            "csr=%x cause=%lx rd=%lx",
            custom_csrs[i], excpt.cause, blocked_read);

        TEST_SETUP_EXCEPT();
        ai_linknan_write_backend_custom_csr(
            custom_csrs[i], opened_custom_csrs[i] ^ writable_probe_masks[i]);
        TEST_ASSERT("S-mode custom CSR write traps as illegal instruction when mstateen0.C is clear",
            excpt.triggered == true && excpt.cause == CAUSE_ILI,
            "csr=%x cause=%lx", custom_csrs[i], excpt.cause);
    }

    goto_priv(PRIV_M);
    for (unsigned i = 0; i < sizeof(custom_csrs) / sizeof(custom_csrs[0]); ++i) {
        TEST_SETUP_EXCEPT();
        const uint64_t after_blocked_write =
            ai_linknan_read_backend_custom_csr(custom_csrs[i], 0ULL);
        TEST_ASSERT("denied S-mode custom CSR write has no architectural side effect",
            excpt.triggered == false &&
            after_blocked_write == opened_custom_csrs[i],
            "csr=%x observed=%lx expected=%lx",
            custom_csrs[i], after_blocked_write, opened_custom_csrs[i]);

        TEST_SETUP_EXCEPT();
        ai_linknan_write_backend_custom_csr(custom_csrs[i], saved_custom_csrs[i]);
        TEST_ASSERT("M-mode restores each backend custom CSR after the state-enable matrix",
            excpt.triggered == false,
            "csr=%x cause=%lx", custom_csrs[i], excpt.cause);
    }
    CSRW(CSR_MSTATEEN0, saved_mstateen0);

    TEST_END(ai_linknan_priv_mstateen0_c_controls_smode_custom_csr_rw_corner);
}
