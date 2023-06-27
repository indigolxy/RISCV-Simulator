
#ifndef RISCV_SIMULATOR_INSTUCTION_H
#define RISCV_SIMULATOR_INSTUCTION_H

#include "../utils/config.h"

enum class InstructionType {
  R, // 0110011
  I, // 00x0011
  S, // 0100011
  B, // 1100011
  U, // 0x10111
  J // 1101111
  // END:0ff00513
};

enum class OptType {
  LUI, AUIPC, // U-type
  JAL, // J-type
  JALR, // I-type
  BEQ, BNE, BLT, BGE, BLTU, BGEU, // B-type
  LB, LH, LW, LBU, LHU, // I-type
  SB, SH, SW, // S-type
  ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI, // I-type
  ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND // R-type
};

class InstructionUnit {
public:
  struct Instruction {
    InstructionType type;
    OptType opt;
    int rs1 = 0;
    int rs2 = 0;
    int rd = 0;
    int imm = 0;
    Instruction() = default;
  };

  /*
   * decode a 32-bit instruction(int right order)
   */
  Instruction DecodeSet(u32 instruction, InstructionType type);

  static InstructionType GetInstructionType(u32 instruction);

  /*
   * calculate next pc according to current_instruction, pc and predictor
   * B-type:jump, J-type:predictor, else pc += 4;
   */
  int NextPc(predictor, pc) {
    if (current_instruction.type != J) pc += 4;
    else {
    // todo 还有非条件跳转B
      pc = instruction_unit.NextPc(current_instruction, predictor.Jump(current_instruction_code));
    }
  }

private:
  Instruction current_instruction;
  u32 current_instruction_code;

  static u8 GetOpt(u32 instruction);
  static int GetRd(u32 instruction);
  static int GetImm(u32 instruction, InstructionType type);
  static int GetRs1(u32 instruction);
  static int GetRs2(u32 instruction);
  static u8 GetFunct3(u32 instruction);
  static u8 GetFunct7(u32 instruction);

  static int SignExtend(u32 src, int len);
};

#endif //RISCV_SIMULATOR_INSTUCTION_H
