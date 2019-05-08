//
// Created by swift on 2019/5/6.
//

#include "inst_arm64.h"
#include "../register/register_a64.h"

using namespace SandHook::Asm;


template<typename InstStruct>
U8 InstructionA64<InstStruct>::size() {
    return sizeof(InstA64);
}

//PC Rel Inst

template<typename Inst>
A64_INST_PC_REL<Inst>::A64_INST_PC_REL(Inst *inst):InstructionA64<Inst>(inst) {
}

template<typename Inst>
A64_INST_PC_REL<Inst>::A64_INST_PC_REL() {}

template<typename Inst>
ADDR A64_INST_PC_REL<Inst>::getImmPCOffsetTarget() {
    return reinterpret_cast<ADDR>(this->getImmPCOffset() + (ADDR) this->getPC());
}

//ADR ADRP

A64_ADR_ADRP::A64_ADR_ADRP() {}

A64_ADR_ADRP::A64_ADR_ADRP(aarch64_adr_adrp *inst) : A64_INST_PC_REL(inst) {
    decode(inst);
}

ADDR A64_ADR_ADRP::getImmPCOffset() {
    U32 hi = get()->immhi;
    U32 lo = get()->immlo;
    ADDR offset = signExtend64(12, COMBINE(hi, lo, IMM_LO_W));
    if (isADRP()) {
        offset *= PAGE_SIZE;
    }
    return offset;
}

ADDR A64_ADR_ADRP::getImmPCOffsetTarget() {
    void * base = AlignDown(getPC(), PAGE_SIZE);
    return reinterpret_cast<ADDR>(getImmPCOffset() + (ADDR) base);
}

int A64_ADR_ADRP::getImm()  {
    return getImmPCOffset();
}

A64_ADR_ADRP::A64_ADR_ADRP(A64_ADR_ADRP::OP op, RegisterA64 *rd, int imme) : op(op), rd(rd),
                                                                             imme(imme) {
    assembler();
}

void A64_ADR_ADRP::decode(aarch64_adr_adrp *decode) {

}

void A64_ADR_ADRP::assembler() {

}


//Mov Wide

A64_MOV_WIDE::A64_MOV_WIDE() {}

A64_MOV_WIDE::A64_MOV_WIDE(aarch64_mov_wide *inst) : InstructionA64(inst) {
    decode(inst);
}

A64_MOV_WIDE::A64_MOV_WIDE(A64_MOV_WIDE::OP op,RegisterA64* rd, U16 imme, U8 shift)
        : shift(shift), op(op), imme(imme), rd(rd) {
    assembler();
}

void A64_MOV_WIDE::assembler() {
    get()->opcode = MOV_WIDE_OPCODE;
    get()->imm16 = imme;
    get()->hw = static_cast<InstA64>(shift / 16);
    get()->opc = op;
    get()->sf = rd->is64Bit() ? 1 : 0;
    get()->rd = rd->getCode();
}

void A64_MOV_WIDE::decode(aarch64_mov_wide *inst) {
    imme = static_cast<U16>(inst->imm16);
    shift = static_cast<U8>(inst->hw * 16);
    op = OP(inst->opc);
    if (inst->sf == 1) {
        rd = XRegister::get(static_cast<U8>(inst->rd));
    } else {
        rd = WRegister::get(static_cast<U8>(inst->rd));
    }
}



//B BL

A64_B_BL::A64_B_BL() {}

A64_B_BL::A64_B_BL(aarch64_b_bl *inst) : A64_INST_PC_REL(inst) {
    decode(inst);
}

A64_B_BL::A64_B_BL(A64_B_BL::OP op, ADDR offset) : op(op), offset(offset) {
    assembler();
}

ADDR A64_B_BL::getImmPCOffset() {
    return signExtend64(26 + 2, COMBINE(get()->imm26, 0b00, 2));
}

void A64_B_BL::decode(aarch64_b_bl *inst) {
    op = OP(inst->op);
    offset = getImmPCOffset();
}

void A64_B_BL::assembler() {
    get()->opcode = B_BL_OPCODE;
    get()->op = op;
    get()->imm26 = TruncateToUint26(offset);
}



//CBZ CBNZ

A64_CBZ_CBNZ::A64_CBZ_CBNZ() {}

A64_CBZ_CBNZ::A64_CBZ_CBNZ(aarch64_cbz_cbnz *inst) : A64_INST_PC_REL(inst) {
    decode(inst);
}

A64_CBZ_CBNZ::A64_CBZ_CBNZ(A64_CBZ_CBNZ::OP op, ADDR offset, RegisterA64 *rt) : op(op),
                                                                                offset(offset),
                                                                                rt(rt) {
    assembler();
}

ADDR A64_CBZ_CBNZ::getImmPCOffset() {
    return signExtend64(19 + 2, COMBINE(get()->imm19, 0b00, 2));
}

void A64_CBZ_CBNZ::decode(aarch64_cbz_cbnz *inst) {
    op = OP(get()->op);
    if (inst->sf == 1) {
        rt = XRegister::get(static_cast<U8>(inst->rt));
    } else {
        rt = WRegister::get(static_cast<U8>(inst->rt));
    }
    offset = getImmPCOffset();
}

void A64_CBZ_CBNZ::assembler() {
    get()->opcode = CBZ_CBNZ_OPCODE;
    get()->op = op;
    get()->rt = rt->getCode();
    get()->sf = rt->is64Bit() ? 1 : 0;
    get()->imm19 = TruncateToUint19(offset);
}


//B.Cond

A64_B_COND::A64_B_COND() {}

A64_B_COND::A64_B_COND(aarch64_b_cond *inst) : A64_INST_PC_REL(inst) {
    decode(inst);
}

A64_B_COND::A64_B_COND(Condition condition, ADDR offset) : condition(condition), offset(offset) {
    assembler();
}

ADDR A64_B_COND::getImmPCOffset() {
    return signExtend64(19 + 2, COMBINE(get()->imm19, 0b00, 2));
}

void A64_B_COND::decode(aarch64_b_cond *inst) {
    condition = Condition(inst->cond);
    offset = getImmPCOffset();
}

void A64_B_COND::assembler() {
    get()->opcode = CBZ_B_COND_OPCODE;
    get()->cond = condition;
    get()->imm19 = TruncateToUint19(offset);
}


//TBZ TBNZ

A64_TBZ_TBNZ::A64_TBZ_TBNZ() {}

A64_TBZ_TBNZ::A64_TBZ_TBNZ(aarch64_tbz_tbnz *inst) : A64_INST_PC_REL(inst) {
    decode(inst);
}

A64_TBZ_TBNZ::A64_TBZ_TBNZ(A64_TBZ_TBNZ::OP op, RegisterA64 *rt, U32 bit, ADDR offset) : op(op),
                                                                                         rt(rt),
                                                                                         bit(bit),
                                                                                         offset(offset) {
    assembler();
}

ADDR A64_TBZ_TBNZ::getImmPCOffset() {
    return signExtend64(14, get()->imm14) << 2;
}

void A64_TBZ_TBNZ::decode(aarch64_tbz_tbnz *inst) {
    bit = COMBINE(inst->b5, inst->b40, 5);
    if (inst->b5 == 1) {
        rt = XRegister::get(static_cast<U8>(inst->rt));
    } else {
        rt = WRegister::get(static_cast<U8>(inst->rt));
    }
    op = OP(inst->op);
    offset = getImmPCOffset();
}

void A64_TBZ_TBNZ::assembler() {
    get()->opcode = TBZ_TBNZ_OPCODE;
    get()->op = op;
    get()->b5 = rt->is64Bit() ? 1 : 0;
    get()->rt = rt->getCode();
    get()->b40 = BITS(bit, sizeof(U32) - 5, sizeof(U32));
    get()->imm14 = TruncateToUint14(offset);
}