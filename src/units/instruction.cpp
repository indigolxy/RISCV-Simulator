#include "instuction.h"
#include <exception>

u8 InstructionUnit::GetOpt(u32 instruction) {
  u32 tmp = 0x7f;
  tmp = tmp & instruction;
  return tmp;
}

int InstructionUnit::GetRd(u32 instruction) {
  u32 tmp = 0xf80;
  tmp = tmp & instruction;
  tmp = tmp >> 7;
  return int(tmp);
}

int InstructionUnit::SignExtend(u32 src, int len) {
  u32 tmp = src >> (len - 1);
  if (tmp == 0) return int(src);
  tmp = 0;
  for (int i = 0; i < len; ++i) {
    tmp = tmp | (1 << i);
  }
  tmp = ~tmp;
  tmp = tmp | src;
  return int(tmp);
}

int InstructionUnit::GetImm(u32 instruction, InstructionType type) {
  u32 ret = 0;
  if (type == InstructionType::U) {
    ret = instruction >> 12;
    ret = ret << 12;
    return int(ret);
  }
  if (type == InstructionType::J) {
    u32 tmp = instruction >> 12;
    ret = ret | (tmp & 0x80000);
    ret = ret | ((tmp & 0x7fe00) >> 9);
    ret = ret | ((tmp & 0x100) << 2);
    ret = ret | ((tmp & 0xff) << 11);
    ret = ret << 1;
    return SignExtend(ret, 21);
  }
  if (type == InstructionType::I) {
    u8 f3 = GetFunct3(instruction);
    if (GetOpt(instruction) == 0b0010011 && (f3 == 0b001 || f3 == 0b101)) {
      ret = (instruction & 0x1f00000) >> 20;
      return int(ret);
    }
    else {
      ret = instruction >> 20;
      return SignExtend(ret, 12);
    }
  }
  if (type == InstructionType::S) {
    u32 tmp = instruction >> 25;
    tmp = tmp << 5;
    ret = ret | tmp;
    tmp = (instruction & 0xf80) >> 7;
    ret = ret | tmp;
    return SignExtend(ret, 12);
  }
  u32 tmp = (instruction >> 31) << 12;
  ret = ret | tmp;
  tmp = (instruction & 0x7e000000) >> 20;
  ret = ret | tmp;
  tmp = (instruction & 0xf00) >> 7;
  ret = ret | tmp;
  tmp = (instruction & 0x80) << 4;
  ret = ret | tmp;
  return SignExtend(ret, 13);
}

int InstructionUnit::GetRs1(u32 instruction) {
  u32 tmp = (instruction & 0xf8000) >> 15;
  return int(tmp);
}

int InstructionUnit::GetRs2(u32 instruction) {
  u32 tmp = (instruction & 0x1f00000) >> 20;
  return int(tmp);
}

u8 InstructionUnit::GetFunct3(u32 instruction) {
  return u8((instruction & 0x7000) >> 12);
}

u8 InstructionUnit::GetFunct7(u32 instruction) {
  return u8((instruction & 0xfe000000) >> 25);
}

InstructionType InstructionUnit::GetInstructionType(u32 instruction) {
  u8 tmp = GetOpt(instruction);
  if (tmp == 0b0110011) return InstructionType::R;
  if (tmp == 0b11 || tmp == 0b10011 || tmp == 0b1100111) return InstructionType::I;
  if (tmp == 0b0100011) return InstructionType::S;
  if (tmp == 0b1100011) return InstructionType::B;
  if (tmp == 0b0110111 || tmp == 0b0010111) return InstructionType::U;
  if (tmp == 0b1101111) return InstructionType::J;
  else throw std::exception();
}

InstructionUnit::Instruction InstructionUnit::DecodeSet(u32 instruction, InstructionType type) {
  InstructionUnit::Instruction ret;
  ret.type = type;
  u8 op_code = GetOpt(instruction);
  switch (type) {
    case InstructionType::U : {
      ret.rd = GetRd(instruction);
      ret.imm = GetImm(instruction, type);
      ret.opt = (op_code == 0b0110111) ? OptType::LUI : OptType::AUIPC;
      break;
    }
    case InstructionType::J : {
      ret.rd = GetRd(instruction);
      ret.imm = GetImm(instruction, type);
      ret.opt = OptType::JAL;
      break;
    }
    case InstructionType::R : {
      ret.rd = GetRd(instruction);
      ret.rs1 = GetRs1(instruction);
      ret.rs2 = GetRs2(instruction);
      u8 f3 = GetFunct3(instruction);
      u8 f7 = GetFunct7(instruction);
      switch (f3) {
        case 0b000 : {
          if (f7 == 0b0000000) {
            ret.opt = OptType::ADD;
          }
          else if (f7 == 0b0100000) {
            ret.opt = OptType::SUB;
          }
          break;
        }
        case 0b001 : {
          ret.opt = OptType::SLL;
          break;
        }
        case 0b010 : {
          ret.opt = OptType::SLT;
          break;
        }
        case 0b011 : {
          ret.opt = OptType::SLTU;
          break;
        }
        case 0b100 : {
          ret.opt = OptType::XOR;
          break;
        }
        case 0b101 : {
          if (f7 == 0b0000000) {
            ret.opt = OptType::SRL;
          }
          else if (f7 == 0b0100000) {
            ret.opt = OptType::SRA;
          }
          break;
        }
        case 0b110 : {
          ret.opt = OptType::OR;
          break;
        }
        case 0b111 : {
          ret.opt = OptType::AND;
          break;
        }
      }
      break;
    }
    case InstructionType::I : {
      ret.rd = GetRd(instruction);
      ret.rs1 = GetRs1(instruction);
      ret.imm = GetImm(instruction, type);
      u8 f3 = GetFunct3(instruction);
      u8 f7 = GetFunct7(instruction);
      if (op_code == 0b1100111) {
        ret.opt = OptType::JALR;
      }
      else if (op_code == 0b0000011) {
        switch (f3) {
          case 0b000 : {
            ret.opt = OptType::LB;
            break;
          }
          case 0b001 : {
            ret.opt = OptType::LH;
            break;
          }
          case 0b010 : {
            ret.opt = OptType::LW;
            break;
          }
          case 0b100 : {
            ret.opt = OptType::LBU;
            break;
          }
          case 0b101 : {
            ret.opt = OptType::LHU;
            break;
          }
        }
      }
      else {
        switch (f3) {
          case 0b000 : {
            ret.opt = OptType::ADDI;
            break;
          }
          case 0b010 : {
            ret.opt = OptType::SLTI;
            break;
          }
          case 0b011 : {
            ret.opt = OptType::SLTIU;
            break;
          }
          case 0b100 : {
            ret.opt = OptType::XORI;
            break;
          }
          case 0b110 : {
            ret.opt = OptType::ORI;
            break;
          }
          case 0b111 : {
            ret.opt = OptType::ANDI;
            break;
          }
          case 0b001 : {
            ret.opt = OptType::SLLI;
            break;
          }
          case 0b101 : {
            if (f7 == 0) ret.opt = OptType::SRLI;
            else ret.opt = OptType::SRAI;
            break;
          }
        }
      }
      break;
    }
    case InstructionType::S : {
      ret.rs1 = GetRs1(instruction);
      ret.rs2 = GetRs2(instruction);
      ret.imm = GetImm(instruction, type);
      u8 f3 = GetFunct3(instruction);
      if (f3 == 0) {
        ret.opt = OptType::SB;
      }
      else if (f3 == 0b001) {
        ret.opt = OptType::SH;
      }
      else {
        ret.opt = OptType::SW;
      }
      break;
    }
    case InstructionType::B : {
      ret.rs1 = GetRs1(instruction);
      ret.rs2 = GetRs2(instruction);
      ret.imm = GetImm(instruction, type);
      u8 f3 = GetFunct3(instruction);
      switch (f3) {
        case 0b000 : {
          ret.opt = OptType::BEQ;
          break;
        }
        case 0b001 : {
          ret.opt = OptType::BNE;
          break;
        }
        case 0b100 : {
          ret.opt = OptType::BLT;
          break;
        }
        case 0b101 : {
          ret.opt = OptType::BGE;
          break;
        }
        case 0b110 : {
          ret.opt = OptType::BLTU;
          break;
        }
        case 0b111 : {
          ret.opt = OptType::BGEU;
          break;
        }
      }
      break;
    }
  }
  current_instruction = ret;
  current_instruction_code = instruction;
  return ret;
}