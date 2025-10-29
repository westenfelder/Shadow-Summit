#include "rvemu.h"

#define QUADRANT(data) (((data) >>  0) & 0x3 )

/**
 * normal types
*/
#define OPCODE(data) (((data) >>  2) & 0x1f)
#define RD(data)     (((data) >>  7) & 0x1f)
#define RS1(data)    (((data) >> 15) & 0x1f)
#define RS2(data)    (((data) >> 20) & 0x1f)
#define RS3(data)    (((data) >> 27) & 0x1f)
#define FUNCT2(data) (((data) >> 25) & 0x3 )
#define FUNCT3(data) (((data) >> 12) & 0x7 )
#define FUNCT7(data) (((data) >> 25) & 0x7f)
#define IMM116(data) (((data) >> 26) & 0x3f)

static FORCE_INLINE insn_t insn_utype_read(u32 data) {
    return (insn_t) {
        .imm = (i32)data & 0xfffff000,
        .rd = RD(data),
    };
}

static FORCE_INLINE insn_t insn_itype_read(u32 data) {
    return (insn_t) {
        .imm = (i32)data >> 20,
        .rs1 = RS1(data),
        .rd = RD(data),
    };
}

static FORCE_INLINE insn_t insn_jtype_read(u32 data) {
    u32 imm20   = (data >> 31) & 0x1;
    u32 imm101  = (data >> 21) & 0x3ff;
    u32 imm11   = (data >> 20) & 0x1;
    u32 imm1912 = (data >> 12) & 0xff;

    i32 imm = (imm20 << 20) | (imm1912 << 12) | (imm11 << 11) | (imm101 << 1);
    imm = (imm << 11) >> 11;

    return (insn_t) {
        .imm = imm,
        .rd = RD(data),
    };
}

static FORCE_INLINE insn_t insn_btype_read(u32 data) {
    u32 imm12  = (data >> 31) & 0x1;
    u32 imm105 = (data >> 25) & 0x3f;
    u32 imm41  = (data >>  8) & 0xf;
    u32 imm11  = (data >>  7) & 0x1;

    i32 imm = (imm12 << 12) | (imm11 << 11) |(imm105 << 5) | (imm41 << 1);
    imm = (imm << 19) >> 19;

    return (insn_t) {
        .imm = imm,
        .rs1 = RS1(data),
        .rs2 = RS2(data),
    };
}

static FORCE_INLINE insn_t insn_rtype_read(u32 data) {
    return (insn_t) {
        .rs1 = RS1(data),
        .rs2 = RS2(data),
        .rd = RD(data),
    };
}

static FORCE_INLINE insn_t insn_stype_read(u32 data) {
    u32 imm115 = (data >> 25) & 0x7f;
    u32 imm40  = (data >>  7) & 0x1f;

    i32 imm = (imm115 << 5) | imm40;
    imm = (imm << 20) >> 20;
    return (insn_t) {
        .imm = imm,
        .rs1 = RS1(data),
        .rs2 = RS2(data),
    };
}

static FORCE_INLINE insn_t insn_csrtype_read(u32 data) {
    return (insn_t) {
        .csr = data >> 20,
        .rs1 = RS1(data),
        .rd =  RD(data),
    };
}

static FORCE_INLINE insn_t insn_fprtype_read(u32 data) {
    return (insn_t) {
        .rs1 = RS1(data),
        .rs2 = RS2(data),
        .rs3 = RS3(data),
        .rd =  RD(data),
    };
}

/**
 * compressed types
*/
#define COPCODE(data)     (((data) >> 13) & 0x7 )
#define CFUNCT1(data)     (((data) >> 12) & 0x1 )
#define CFUNCT2LOW(data)  (((data) >>  5) & 0x3 )
#define CFUNCT2HIGH(data) (((data) >> 10) & 0x3 )
#define RP1(data)         (((data) >>  7) & 0x7 )
#define RP2(data)         (((data) >>  2) & 0x7 )
#define RC1(data)         (((data) >>  7) & 0x1f)
#define RC2(data)         (((data) >>  2) & 0x1f)

static FORCE_INLINE insn_t insn_catype_read(u16 data) {
    return (insn_t) {
        .rd = RP1(data) + 8,
        .rs2 = RP2(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_crtype_read(u16 data) {
    return (insn_t) {
        .rs1 = RC1(data),
        .rs2 = RC2(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_citype_read(u16 data) {
    u32 imm40 = (data >>  2) & 0x1f;
    u32 imm5  = (data >> 12) & 0x1;
    i32 imm = (imm5 << 5) | imm40;
    imm = (imm << 26) >> 26;

    return (insn_t) {
        .imm = imm,
        .rd = RC1(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_citype_read2(u16 data) {
    u32 imm86 = (data >>  2) & 0x7;
    u32 imm43 = (data >>  5) & 0x3;
    u32 imm5  = (data >> 12) & 0x1;

    i32 imm = (imm86 << 6) | (imm43 << 3) | (imm5 << 5);

    return (insn_t) {
        .imm = imm,
        .rd = RC1(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_citype_read3(u16 data) {
    u32 imm5  = (data >>  2) & 0x1;
    u32 imm87 = (data >>  3) & 0x3;
    u32 imm6  = (data >>  5) & 0x1;
    u32 imm4  = (data >>  6) & 0x1;
    u32 imm9  = (data >> 12) & 0x1;

    i32 imm = (imm5 << 5) | (imm87 << 7) | (imm6 << 6) | (imm4 << 4) | (imm9 << 9);
    imm = (imm << 22) >> 22;

    return (insn_t) {
        .imm = imm,
        .rd = RC1(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_citype_read4(u16 data) {
    u32 imm5  = (data >> 12) & 0x1;
    u32 imm42 = (data >>  4) & 0x7;
    u32 imm76 = (data >>  2) & 0x3;

    i32 imm = (imm5 << 5) | (imm42 << 2) | (imm76 << 6);

    return (insn_t) {
        .imm = imm,
        .rd = RC1(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_citype_read5(u16 data) {
    u32 imm1612 = (data >>  2) & 0x1f;
    u32 imm17   = (data >> 12) & 0x1;

    i32 imm = (imm1612 << 12) | (imm17 << 17);
    imm = (imm << 14) >> 14;
    return (insn_t) {
        .imm = imm,
        .rd = RC1(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cbtype_read(u16 data) {
    u32 imm5  = (data >>  2) & 0x1;
    u32 imm21 = (data >>  3) & 0x3;
    u32 imm76 = (data >>  5) & 0x3;
    u32 imm43 = (data >> 10) & 0x3;
    u32 imm8  = (data >> 12) & 0x1;

    i32 imm = (imm8 << 8) | (imm76 << 6) | (imm5 << 5) | (imm43 << 3) | (imm21 << 1);
    imm = (imm << 23) >> 23;

    return (insn_t) {
        .imm = imm,
        .rs1 = RP1(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cbtype_read2(u16 data) {
    u32 imm40 = (data >>  2) & 0x1f;
    u32 imm5  = (data >> 12) & 0x1;
    i32 imm = (imm5 << 5) | imm40;
    imm = (imm << 26) >> 26;

    return (insn_t) {
        .imm = imm,
        .rd = RP1(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cstype_read(u16 data) {
    u32 imm76 = (data >>  5) & 0x3;
    u32 imm53 = (data >> 10) & 0x7;

    i32 imm = ((imm76 << 6) | (imm53 << 3));

    return (insn_t) {
        .imm = imm,
        .rs1 = RP1(data) + 8,
        .rs2 = RP2(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cstype_read2(u16 data) {
    u32 imm6  = (data >>  5) & 0x1;
    u32 imm2  = (data >>  6) & 0x1;
    u32 imm53 = (data >> 10) & 0x7;

    i32 imm = ((imm6 << 6) | (imm2 << 2) | (imm53 << 3));

    return (insn_t) {
        .imm = imm,
        .rs1 = RP1(data) + 8,
        .rs2 = RP2(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cjtype_read(u16 data) {
    u32 imm5  = (data >>  2) & 0x1;
    u32 imm31 = (data >>  3) & 0x7;
    u32 imm7  = (data >>  6) & 0x1;
    u32 imm6  = (data >>  7) & 0x1;
    u32 imm10 = (data >>  8) & 0x1;
    u32 imm98 = (data >>  9) & 0x3;
    u32 imm4  = (data >> 11) & 0x1;
    u32 imm11 = (data >> 12) & 0x1;

    i32 imm = ((imm5 << 5) | (imm31 << 1) | (imm7 << 7) | (imm6 << 6) |
               (imm10 << 10) | (imm98 << 8) | (imm4 << 4) | (imm11 << 11));
    imm = (imm << 20) >> 20;
    return (insn_t) {
        .imm = imm,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cltype_read(u16 data) {
    u32 imm6  = (data >>  5) & 0x1;
    u32 imm2  = (data >>  6) & 0x1;
    u32 imm53 = (data >> 10) & 0x7;

    i32 imm = (imm6 << 6) | (imm2 << 2) | (imm53 << 3);

    return (insn_t) {
        .imm = imm,
        .rs1 = RP1(data) + 8,
        .rd  = RP2(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_cltype_read2(u16 data) {
    u32 imm76 = (data >>  5) & 0x3;
    u32 imm53 = (data >> 10) & 0x7;

    i32 imm = (imm76 << 6) | (imm53 << 3);

    return (insn_t) {
        .imm = imm,
        .rs1 = RP1(data) + 8,
        .rd  = RP2(data) + 8,
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_csstype_read(u16 data) {
    u32 imm86 = (data >>  7) & 0x7;
    u32 imm53 = (data >> 10) & 0x7;

    i32 imm = (imm86 << 6) | (imm53 << 3);

    return (insn_t) {
        .imm = imm,
        .rs2 = RC2(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_csstype_read2(u16 data) {
    u32 imm76 = (data >> 7) & 0x3;
    u32 imm52 = (data >> 9) & 0xf;

    i32 imm = (imm76 << 6) | (imm52 << 2);

    return (insn_t) {
        .imm = imm,
        .rs2 = RC2(data),
        .rvc = true,
    };
}

static FORCE_INLINE insn_t insn_ciwtype_read(u16 data) {
    u32 imm3  = (data >>  5) & 0x1;
    u32 imm2  = (data >>  6) & 0x1;
    u32 imm96 = (data >>  7) & 0xf;
    u32 imm54 = (data >> 11) & 0x3;

    i32 imm = (imm3 << 3) | (imm2 << 2) | (imm96 << 6) | (imm54 << 4);

    return (insn_t) {
        .imm = imm,
        .rd = RP2(data) + 8,
        .rvc = true,
    };
}

void insn_decode(insn_t *insn, u32 data) {
    u32 quadrant = QUADRANT(data);
    switch (quadrant) {
    case 0x0: {
        u32 copcode = COPCODE(data);

        switch (copcode) {
        case 0x0: /* C.ADDI4SPN */
            *insn = insn_ciwtype_read(data);
            insn->rs1 = sp;
            insn->type = insn_addi;
            assert(insn->imm != 0);
            return;
        case 0x1: /* C.FLD */
            *insn = insn_cltype_read2(data);
            insn->type = insn_fld;
            return;
        case 0x2: /* C.LW */
            *insn = insn_cltype_read(data);
            insn->type = insn_lw;
            return;
        case 0x3: /* C.LD */
            *insn = insn_cltype_read2(data);
            insn->type = insn_ld;
            return;
        case 0x5: /* C.FSD */
            *insn = insn_cstype_read(data);
            insn->type = insn_fsd;
            return;
        case 0x6: /* C.SW */
            *insn = insn_cstype_read2(data);
            insn->type = insn_sw;
            return;
        case 0x7: /* C.SD */
            *insn = insn_cstype_read(data);
            insn->type = insn_sd;
            return;
        default: printf("data: %x\n", data); fatal("unimplemented");
        }
    }
    unreachable();
    case 0x1: {
        u32 copcode = COPCODE(data);

        switch (copcode) {
        case 0x0: /* C.ADDI */
            *insn = insn_citype_read(data);
            insn->rs1 = insn->rd;
            insn->type = insn_addi;
            return;
        case 0x1: /* C.ADDIW */
            *insn = insn_citype_read(data);
            assert(insn->rd != 0);
            insn->rs1 = insn->rd;
            insn->type = insn_addiw;
            return;
        case 0x2: /* C.LI */
            *insn = insn_citype_read(data);
            insn->rs1 = zero;
            insn->type = insn_addi;
            return;
        case 0x3: {
            i32 rd = RC1(data);
            if (rd == 2) { /* C.ADDI16SP */
                *insn = insn_citype_read3(data);
                assert(insn->imm != 0);
                insn->rs1 = insn->rd;
                insn->type = insn_addi;
                return;
            } else { /* C.LUI */
                *insn = insn_citype_read5(data);
                assert(insn->imm != 0);
                insn->type = insn_lui;
                return;
            }
        }
        unreachable();
        case 0x4: {
            u32 cfunct2high = CFUNCT2HIGH(data);

            switch (cfunct2high) {
            case 0x0:   /* C.SRLI */
            case 0x1:   /* C.SRAI */
            case 0x2: { /* C.ANDI */
                *insn = insn_cbtype_read2(data);
                insn->rs1 = insn->rd;

                if (cfunct2high == 0x0) {
                    insn->type = insn_srli;
                } else if (cfunct2high == 0x1) {
                    insn->type = insn_srai;
                } else {
                    insn->type = insn_andi;
                }
                return;
            }
            unreachable();
            case 0x3: {
                u32 cfunct1 = CFUNCT1(data);

                switch (cfunct1) {
                case 0x0: {
                    u32 cfunct2low = CFUNCT2LOW(data);

                    *insn = insn_catype_read(data);
                    insn->rs1 = insn->rd;

                    switch (cfunct2low) {
                    case 0x0: /* C.SUB */
                        insn->type = insn_sub;
                        break;
                    case 0x1: /* C.XOR */
                        insn->type = insn_xor;
                        break;
                    case 0x2: /* C.OR */
                        insn->type = insn_or;
                        break;
                    case 0x3: /* C.AND */
                        insn->type = insn_and;
                        break;
                    default: unreachable();
                    }
                    return;
                }
                unreachable();
                case 0x1: {
                    u32 cfunct2low = CFUNCT2LOW(data);

                    *insn = insn_catype_read(data);
                    insn->rs1 = insn->rd;

                    switch (cfunct2low) {
                    case 0x0: /* C.SUBW */
                        insn->type = insn_subw;
                        break;
                    case 0x1: /* C.ADDW */
                        insn->type = insn_addw;
                        break;
                    default: unreachable();
                    }
                    return;
                }
                unreachable();
                default: unreachable();
                }
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0x5: /* C.J */
            *insn = insn_cjtype_read(data);
            insn->rd = zero;
            insn->type = insn_jal;
            insn->cont = true;
            return;
        case 0x6: /* C.BEQZ */
        case 0x7: /* C.BNEZ */
            *insn = insn_cbtype_read(data);
            insn->rs2 = zero;
            insn->type = copcode == 0x6 ? insn_beq : insn_bne;
            return;
        default: fatal("unrecognized copcode");
        }
    }
    unreachable();
    case 0x2: {
        u32 copcode = COPCODE(data);
        switch (copcode) {
        case 0x0: /* C.SLLI */
            *insn = insn_citype_read(data);
            insn->rs1 = insn->rd;
            insn->type = insn_slli;
            return;
        case 0x1: /* C.FLDSP */
            *insn = insn_citype_read2(data);
            insn->rs1 = sp;
            insn->type = insn_fld;
            return;
        case 0x2: /* C.LWSP */
            *insn = insn_citype_read4(data);
            assert(insn->rd != 0);
            insn->rs1 = sp;
            insn->type = insn_lw;
            return;
        case 0x3: /* C.LDSP */
            *insn = insn_citype_read2(data);
            assert(insn->rd != 0);
            insn->rs1 = sp;
            insn->type = insn_ld;
            return;
        case 0x4: {
            u32 cfunct1 = CFUNCT1(data);

            switch (cfunct1) {
            case 0x0: {
                *insn = insn_crtype_read(data);

                if (insn->rs2 == 0) { /* C.JR */
                    assert(insn->rs1 != 0);
                    insn->rd = zero;
                    insn->type = insn_jalr;
                    insn->cont = true;
                } else { /* C.MV */
                    insn->rd = insn->rs1;
                    insn->rs1 = zero;
                    insn->type = insn_add;
                }
                return;
            }
            unreachable();
            case 0x1: {
                *insn = insn_crtype_read(data);
                if (insn->rs1 == 0 && insn->rs2 == 0) { /* C.EBREAK */
                    fatal("unimplmented");
                } else if (insn->rs2 == 0) { /* C.JALR */
                    insn->rd = ra;
                    insn->type = insn_jalr;
                    insn->cont = true;
                } else { /* C.ADD */
                    insn->rd = insn->rs1;
                    insn->type = insn_add;
                }
                return;
            }
            unreachable();
            default: unreachable();
            }
        }
        unreachable();
        case 0x5: /* C.FSDSP */
            *insn = insn_csstype_read(data);
            insn->rs1 = sp;
            insn->type = insn_fsd;
            return;
        case 0x6: /* C.SWSP */
            *insn = insn_csstype_read2(data);
            insn->rs1 = sp;
            insn->type = insn_sw;
            return;
        case 0x7: /* C.SDSP */
            *insn = insn_csstype_read(data);
            insn->rs1 = sp;
            insn->type = insn_sd;
            return;
        default: fatal("unrecognized copcode");
        }
    }
    unreachable();
    // HERE
    case 0x3: {
        u32 opcode = OPCODE(data);
        u32 funct3 = FUNCT3(data);
        u32 funct7 = FUNCT7(data);
        u32 funct2 = FUNCT2(data);
        u32 imm116 = IMM116(data);
        u32 rs2 = RS2(data);

        if (opcode == 0x0 && funct3 == 0x0) { // LOAD, LB
            *insn = insn_itype_read(data);
            insn->type = insn_lb;
            return;
        } else if (opcode == 0x0 && funct3 == 0x1) { // LOAD, LH
            *insn = insn_itype_read(data);
            insn->type = insn_lh;
            return;
        } else if (opcode == 0x0 && funct3 == 0x2) { // LOAD, LW
            *insn = insn_itype_read(data);
            insn->type = insn_lw;
            return;
        } else if (opcode == 0x0 && funct3 == 0x3) { // LOAD, LD
            *insn = insn_itype_read(data);
            insn->type = insn_ld;
            return;
        } else if (opcode == 0x0 && funct3 == 0x4) { // LOAD, LBU
            *insn = insn_itype_read(data);
            insn->type = insn_lbu;
            return;
        } else if (opcode == 0x0 && funct3 == 0x5) { // LOAD, LHU
            *insn = insn_itype_read(data);
            insn->type = insn_lhu;
            return;
        } else if (opcode == 0x0 && funct3 == 0x6) { // LOAD, LWU
            *insn = insn_itype_read(data);
            insn->type = insn_lwu;
            return;
        } else if (opcode == 0x1 && funct3 == 0x2) { // FLW
            *insn = insn_itype_read(data);
            insn->type = insn_flw;
            return;
        } else if (opcode == 0x1 && funct3 == 0x3) { // FLD
            *insn = insn_itype_read(data);
            insn->type = insn_fld;
            return;
        } else if (opcode == 0x3 && funct3 == 0x0) { // FENCE
            insn_t _insn = {0};
            *insn = _insn;
            insn->type = insn_fence;
            return;
        } else if (opcode == 0x3 && funct3 == 0x1) { // FENCE.I
            insn_t _insn = {0};
            *insn = _insn;
            insn->type = insn_fence_i;
            return;
        } else if (opcode == 0x4 && funct3 == 0x0) { // OP-IMM, ADDI
            *insn = insn_itype_read(data);
            insn->type = insn_addi;
            return;
        } else if (opcode == 0x4 && funct3 == 0x1 && imm116 == 0) { // OP-IMM, SLLI
            *insn = insn_itype_read(data);
            insn->type = insn_slli;
            return;
        } else if (opcode == 0x4 && funct3 == 0x2) { // OP-IMM, SLTI
            *insn = insn_itype_read(data);
            insn->type = insn_slti;
            return;
        } else if (opcode == 0x4 && funct3 == 0x3) { // OP-IMM, SLTIU
            *insn = insn_itype_read(data);
            insn->type = insn_sltiu;
            return;
        } else if (opcode == 0x4 && funct3 == 0x4) { // OP-IMM, XORI
            *insn = insn_itype_read(data);
            insn->type = insn_xori;
            return;
        } else if (opcode == 0x4 && funct3 == 0x5 && imm116 == 0x0) { // OP-IMM, SRLI
            *insn = insn_itype_read(data);
            insn->type = insn_srli;
            return;
        } else if (opcode == 0x4 && funct3 == 0x5 && imm116 == 0x10) { // OP-IMM, SRAI
            *insn = insn_itype_read(data);
            insn->type = insn_srai;
            return;
        } else if (opcode == 0x4 && funct3 == 0x6) { // OP-IMM, ORI
            *insn = insn_itype_read(data);
            insn->type = insn_ori;
            return;
        } else if (opcode == 0x4 && funct3 == 0x7) { // OP-IMM, ANDI
            *insn = insn_itype_read(data);
            insn->type = insn_andi;
            return;
        } else if (opcode == 0x5) { // AUIPC
            *insn = insn_utype_read(data);
            insn->type = insn_auipc;
            return;
        } else if (opcode == 0x6 && funct3 == 0x0) { // ADDIW
            *insn = insn_itype_read(data);
            insn->type = insn_addiw;
            return;
        } else if (opcode == 0x6 && funct3 == 0x1) { // SLLIW
            *insn = insn_itype_read(data);
            assert(funct7 == 0);
            insn->type = insn_slliw;
            return;
        } else if (opcode == 0x6 && funct3 == 0x5 && funct7 == 0x0) { // SRLIW
            *insn = insn_itype_read(data);
            insn->type = insn_srliw;
            return;
        } else if (opcode == 0x6 && funct3 == 0x5 && funct7 == 0x20) { // SRAIW
            *insn = insn_itype_read(data);
            insn->type = insn_sraiw;
            return;
        } else if (opcode == 0x8 && funct3 == 0x0) { // STORE, SB
            *insn = insn_stype_read(data);
            insn->type = insn_sb;
            return;
        } else if (opcode == 0x8 && funct3 == 0x1) { // STORE, SH
            *insn = insn_stype_read(data);
            insn->type = insn_sh;
            return;
        } else if (opcode == 0x8 && funct3 == 0x2) { // STORE, SW
            *insn = insn_stype_read(data);
            insn->type = insn_sw;
            return;
        } else if (opcode == 0x8 && funct3 == 0x3) { // STORE, SD
            *insn = insn_stype_read(data);
            insn->type = insn_sd;
            return;
        } else if (opcode == 0x9 && funct3 == 0x2) { // FSW
            *insn = insn_stype_read(data);
            insn->type = insn_fsw;
            return;
        } else if (opcode == 0x9 && funct3 == 0x3) { // FSD
            *insn = insn_stype_read(data);
            insn->type = insn_fsd;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x0) { // OP, ADD
            *insn = insn_rtype_read(data);
            insn->type = insn_add;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x1) { // OP, SLL
            *insn = insn_rtype_read(data);
            insn->type = insn_sll;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x2) { // OP, SLT
            *insn = insn_rtype_read(data);
            insn->type = insn_slt;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x3) { // OP, SLTU
            *insn = insn_rtype_read(data);
            insn->type = insn_sltu;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x4) { // OP, XOR
            *insn = insn_rtype_read(data);
            insn->type = insn_xor;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x5) { // OP, SRL
            *insn = insn_rtype_read(data);
            insn->type = insn_srl;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x6) { // OP, OR
            *insn = insn_rtype_read(data);
            insn->type = insn_or;
            return;
        } else if (opcode == 0xc && funct7 == 0x0 && funct3 == 0x7) { // OP, AND
            *insn = insn_rtype_read(data);
            insn->type = insn_and;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x0) { // OP, MUL
            *insn = insn_rtype_read(data);
            insn->type = insn_mul;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x1) { // OP, MULH
            *insn = insn_rtype_read(data);
            insn->type = insn_mulh;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x2) { // OP, MULHSU
            *insn = insn_rtype_read(data);
            insn->type = insn_mulhsu;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x3) { // OP, MULHU
            *insn = insn_rtype_read(data);
            insn->type = insn_mulhu;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x4) { // OP, DIV
            *insn = insn_rtype_read(data);
            insn->type = insn_div;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x5) { // OP, DIVU
            *insn = insn_rtype_read(data);
            insn->type = insn_divu;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x6) { // OP, REM
            *insn = insn_rtype_read(data);
            insn->type = insn_rem;
            return;
        } else if (opcode == 0xc && funct7 == 0x1 && funct3 == 0x7) { // OP, REMU
            *insn = insn_rtype_read(data);
            insn->type = insn_remu;
            return;
        } else if (opcode == 0xc && funct7 == 0x20 && funct3 == 0x0) { // OP, SUB
            *insn = insn_rtype_read(data);
            insn->type = insn_sub;
            return;
        } else if (opcode == 0xc && funct7 == 0x20 && funct3 == 0x5) { // OP, SRA
            *insn = insn_rtype_read(data);
            insn->type = insn_sra;
            return;
        } else if (opcode == 0xd) { // LUI
            *insn = insn_utype_read(data);
            insn->type = insn_lui;
            return;
        } else if (opcode == 0xe && funct7 == 0x0 && funct3 == 0x0) { // ADDW
            *insn = insn_rtype_read(data);
            insn->type = insn_addw;
            return;
        } else if (opcode == 0xe && funct7 == 0x0 && funct3 == 0x1) { // SLLW
            *insn = insn_rtype_read(data);
            insn->type = insn_sllw;
            return;
        } else if (opcode == 0xe && funct7 == 0x0 && funct3 == 0x5) { // SRLW
            *insn = insn_rtype_read(data);
            insn->type = insn_srlw;
            return;
        } else if (opcode == 0xe && funct7 == 0x1 && funct3 == 0x0) { // MULW
            *insn = insn_rtype_read(data);
            insn->type = insn_mulw;
            return;
        } else if (opcode == 0xe && funct7 == 0x1 && funct3 == 0x4) { // DIVW
            *insn = insn_rtype_read(data);
            insn->type = insn_divw;
            return;
        } else if (opcode == 0xe && funct7 == 0x1 && funct3 == 0x5) { // DIVUW
            *insn = insn_rtype_read(data);
            insn->type = insn_divuw;
            return;
        } else if (opcode == 0xe && funct7 == 0x1 && funct3 == 0x6) { // REMW
            *insn = insn_rtype_read(data);
            insn->type = insn_remw;
            return;
        } else if (opcode == 0xe && funct7 == 0x1 && funct3 == 0x7) { // REMUW
            *insn = insn_rtype_read(data);
            insn->type = insn_remuw;
            return;
        } else if (opcode == 0xe && funct7 == 0x20 && funct3 == 0x0) { // SUBW
            *insn = insn_rtype_read(data);
            insn->type = insn_subw;
            return;
        } else if (opcode == 0xe && funct7 == 0x20 && funct3 == 0x5) { // SRAW
            *insn = insn_rtype_read(data);
            insn->type = insn_sraw;
            return;
        } else if (opcode == 0x10 && funct2 == 0x0) { // FMADD.S
            *insn = insn_fprtype_read(data);
            insn->type = insn_fmadd_s;
            return;
        } else if (opcode == 0x10 && funct2 == 0x1) { // FMADD.D
            *insn = insn_fprtype_read(data);
            insn->type = insn_fmadd_d;
            return;
        } else if (opcode == 0x11 && funct2 == 0x0) { // FMSUB.S
            *insn = insn_fprtype_read(data);
            insn->type = insn_fmsub_s;
            return;
        } else if (opcode == 0x11 && funct2 == 0x1) { // FMSUB.D
            *insn = insn_fprtype_read(data);
            insn->type = insn_fmsub_d;
            return;
        } else if (opcode == 0x12 && funct2 == 0x0) { // FNMSUB.S
            *insn = insn_fprtype_read(data);
            insn->type = insn_fnmsub_s;
            return;
        } else if (opcode == 0x12 && funct2 == 0x1) { // FNMSUB.D
            *insn = insn_fprtype_read(data);
            insn->type = insn_fnmsub_d;
            return;
        } else if (opcode == 0x13 && funct2 == 0x0) { // FNMADD.S
            *insn = insn_fprtype_read(data);
            insn->type = insn_fnmadd_s;
            return;
        } else if (opcode == 0x13 && funct2 == 0x1) { // FNMADD.D
            *insn = insn_fprtype_read(data);
            insn->type = insn_fnmadd_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x0) { // FADD.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fadd_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x1) { // FADD.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fadd_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x4) { // FSUB.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fsub_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x5) { // FSUB.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fsub_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x8) { // FMUL.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fmul_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x9) { // FMUL.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fmul_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0xc) { // FDIV.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fdiv_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0xd) { // FDIV.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fdiv_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x10 && funct3 == 0x0) { // FSGNJ.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnj_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x10 && funct3 == 0x1) { // FSGNJN.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnjn_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x10 && funct3 == 0x2) { // FSGNJX.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnjx_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x11 && funct3 == 0x0) { // FSGNJ.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnj_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x11 && funct3 == 0x1) { // FSGNJN.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnjn_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x11 && funct3 == 0x2) { // FSGNJX.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fsgnjx_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x14 && funct3 == 0x0) { // FMIN.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fmin_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x14 && funct3 == 0x1) { // FMAX.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fmax_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x15 && funct3 == 0x0) { // FMIN.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fmin_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x15 && funct3 == 0x1) { // FMAX.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fmax_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x20) { // FCVT.S.D
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 1);
            insn->type = insn_fcvt_s_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x21) { // FCVT.D.S
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0);
            insn->type = insn_fcvt_d_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x2c) { // FSQRT.S
            *insn = insn_rtype_read(data);
            assert(insn->rs2 == 0);
            insn->type = insn_fsqrt_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x2d) { // FSQRT.D
            *insn = insn_rtype_read(data);
            assert(insn->rs2 == 0);
            insn->type = insn_fsqrt_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x50 && funct3 == 0x0) { // FLE.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fle_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x50 && funct3 == 0x1) { // FLT.S
            *insn = insn_rtype_read(data);
            insn->type = insn_flt_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x50 && funct3 == 0x2) { // FEQ.S
            *insn = insn_rtype_read(data);
            insn->type = insn_feq_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x51 && funct3 == 0x0) { // FLE.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fle_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x51 && funct3 == 0x1) { // FLT.D
            *insn = insn_rtype_read(data);
            insn->type = insn_flt_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x51 && funct3 == 0x2) { // FEQ.D
            *insn = insn_rtype_read(data);
            insn->type = insn_feq_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x60 && rs2 == 0x0) { // FCVT.W.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_w_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x60 && rs2 == 0x1) { // FCVT.WU.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_wu_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x60 && rs2 == 0x2) { // FCVT.L.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_l_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x60 && rs2 == 0x3) { // FCVT.LU.S
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_lu_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x61 && rs2 == 0x0) { // FCVT.W.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_w_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x61 && rs2 == 0x1) { // FCVT.WU.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_wu_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x61 && rs2 == 0x2) { // FCVT.L.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_l_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x61 && rs2 == 0x3) { // FCVT.LU.D
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_lu_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x68 && rs2 == 0x0) { // FCVT.S.W
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_s_w;
            return;
        } else if (opcode == 0x14 && funct7 == 0x68 && rs2 == 0x1) { // FCVT.S.WU
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_s_wu;
            return;
        } else if (opcode == 0x14 && funct7 == 0x68 && rs2 == 0x2) { // FCVT.S.L
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_s_l;
            return;
        } else if (opcode == 0x14 && funct7 == 0x68 && rs2 == 0x3) { // FCVT.S.LU
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_s_lu;
            return;
        } else if (opcode == 0x14 && funct7 == 0x69 && rs2 == 0x0) { // FCVT.D.W
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_d_w;
            return;
        } else if (opcode == 0x14 && funct7 == 0x69 && rs2 == 0x1) { // FCVT.D.WU
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_d_wu;
            return;
        } else if (opcode == 0x14 && funct7 == 0x69 && rs2 == 0x2) { // FCVT.D.L
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_d_l;
            return;
        } else if (opcode == 0x14 && funct7 == 0x69 && rs2 == 0x3) { // FCVT.D.LU
            *insn = insn_rtype_read(data);
            insn->type = insn_fcvt_d_lu;
            return;
        } else if (opcode == 0x14 && funct7 == 0x70 && funct3 == 0x0) { // FMV.X.W
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0);
            insn->type = insn_fmv_x_w;
            return;
        } else if (opcode == 0x14 && funct7 == 0x70 && funct3 == 0x1) { // FCLASS.S
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0);
            insn->type = insn_fclass_s;
            return;
        } else if (opcode == 0x14 && funct7 == 0x71 && funct3 == 0x0) { // FMV.X.D
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0);
            insn->type = insn_fmv_x_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x71 && funct3 == 0x1) { // FCLASS.D
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0);
            insn->type = insn_fclass_d;
            return;
        } else if (opcode == 0x14 && funct7 == 0x78) { // FMV_W_X
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0 && FUNCT3(data) == 0);
            insn->type = insn_fmv_w_x;
            return;
        } else if (opcode == 0x14 && funct7 == 0x79) { // FMV_D_X
            *insn = insn_rtype_read(data);
            assert(RS2(data) == 0 && FUNCT3(data) == 0);
            insn->type = insn_fmv_d_x;
            return;
        } else if (opcode == 0x18 && funct3 == 0x0) { // BRANCH, BEQ
            *insn = insn_btype_read(data);
            insn->type = insn_beq;
            return;
        } else if (opcode == 0x18 && funct3 == 0x1) { // BRANCH, BNE
            *insn = insn_btype_read(data);
            insn->type = insn_bne;
            return;
        } else if (opcode == 0x18 && funct3 == 0x4) { // BRANCH, BLT
            *insn = insn_btype_read(data);
            insn->type = insn_blt;
            return;
        } else if (opcode == 0x18 && funct3 == 0x5) { // BRANCH, BGE
            *insn = insn_btype_read(data);
            insn->type = insn_bge;
            return;
        } else if (opcode == 0x18 && funct3 == 0x6) { // BRANCH, BLTU
            *insn = insn_btype_read(data);
            insn->type = insn_bltu;
            return;
        } else if (opcode == 0x18 && funct3 == 0x7) { // BRANCH, BGEU
            *insn = insn_btype_read(data);
            insn->type = insn_bgeu;
            return;
        } else if (opcode == 0x19) { // JALR
            *insn = insn_itype_read(data);
            insn->type = insn_jalr;
            insn->cont = true;
            return;
        } else if (opcode == 0x1b) { // JAL
            *insn = insn_jtype_read(data);
            insn->type = insn_jal;
            insn->cont = true;
            return;
        } else if (opcode == 0x1c && data == 0x73) { // SYSTEM, ECALL
            insn->type = insn_ecall;
            insn->cont = true;
            return;
        } else if (opcode == 0x1c && funct3 == 0x1) { // SYSTEM, CSRRW
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrw;
            return;
        } else if (opcode == 0x1c && funct3 == 0x2) { // SYSTEM, CSRRS
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrs;
            return;
        } else if (opcode == 0x1c && funct3 == 0x3) { // SYSTEM, CSRRC
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrc;
            return;
        } else if (opcode == 0x1c && funct3 == 0x5) { // SYSTEM, CSRRWI
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrwi;
            return;
        } else if (opcode == 0x1c && funct3 == 0x6) { // SYSTEM, CSRRSI
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrsi;
            return;
        } else if (opcode == 0x1c && funct3 == 0x7) { // SYSTEM, CSRRCI
            *insn = insn_csrtype_read(data);
            insn->type = insn_csrrci;
            return;
        } else {
            unreachable();
        }
    }
    unreachable();
    default: unreachable();
    }
}
