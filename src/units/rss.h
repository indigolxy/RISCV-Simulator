
#ifndef RISCV_SIMULATOR_RSS_H
#define RISCV_SIMULATOR_RSS_H

#include "instuction.h"
#include "../utils/config.h"
#include "register.h"
#include "alu.h"
#include "bus.h"
#include "../storage/lsb.h"

class ReservationStation {
private:
  struct RssEntry {
    OptType opt;
    int value1 = 0, value2 = 0;
    int dependency1 = -1, dependency2 = -1;
    int label = 0; // in RoB
    int imm = 0;
  };
public:
  ReservationStation() = default;

  void flush() {
    for (int i = 0; i < RSSSIZE; ++i) {
      rss_now[i] = rss_next[i];
    }
    size_now = size_next;
  }

  bool full() const {return size_now == RSSSIZE;}

  /*
   * read values and dependency in reg
   * add an entry
   */
  void issue(int rob_index, const InstructionUnit::Instruction &ins, const Register &reg) {
    RssEntry tmp;
    tmp.label = rob_index;
    tmp.opt = ins.opt;
    if (ins.type != InstructionType::R) {
      tmp.imm = ins.imm;
    }
    if (ins.type != InstructionType::U) {
      std::pair<int, int> rs = reg.GetValueDependency(ins.rs1);
      tmp.value1 = rs.first;
      tmp.dependency1 = rs.second;
    }
    if (ins.type == InstructionType::R || ins.type == InstructionType::S || ins.type == InstructionType::B) {
      std::pair<int, int> rs = reg.GetValueDependency(ins.rs2);
      tmp.value2 = rs.first;
      tmp.dependency2 = rs.second;
    }
    rss_next[size_next++] = tmp;
  }

  /*
   * find an entry without dependency and calculate in ALU and get result
   * put the information into bus(label, value)
   * remove entry
   */
  void AriExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, int pc) {
    int index = FindIndependentEntry();
    if (index == -1) return; // all entries are not prepared
    int value = 0;
    const RssEntry &tmp = rss_now[index];
    switch (tmp.opt) {
      case OptType::JAL:
      case OptType::LUI : {
        value = tmp.imm;
        break;
      }
      case OptType::AUIPC : {
        value = alu.ADD(pc, tmp.imm);
        break;
      }
      case OptType::JALR : {
        value = alu.AND(alu.ADD(tmp.imm, tmp.value1), ~1);
        break;
      }
      case OptType::BEQ : {
        value = (alu.IsEqual(tmp.value1, tmp.value2)) ? tmp.imm : 4;
        break;
      }
      case OptType::BNE : {
        value = (alu.IsEqual(tmp.value1, tmp.value2)) ? 4 : tmp.imm;
        break;
      }
      case OptType::BLT : {
        value = (alu.IsLessThanSigned(tmp.value1, tmp.value2)) ? tmp.imm : 4;
        break;
      }
      case OptType::BGE : {
        value = (alu.IsLessThanSigned(tmp.value1, tmp.value2)) ? 4 : tmp.imm;
        break;
      }
      case OptType::BLTU : {
        value = (alu.IsLessThanUnsigned(tmp.value1, tmp.value2)) ? tmp.imm : 4;
        break;
      }
      case OptType::BGEU : {
        value = (alu.IsLessThanUnsigned(tmp.value1, tmp.value2)) ? 4 : tmp.imm;
        break;
      }
      case OptType::ADDI : {
        value = alu.ADD(tmp.value1, tmp.imm);
        break;
      }
      case OptType::SLTI : {
        value = alu.IsLessThanSigned(tmp.value1, tmp.imm);
        break;
      }
      case OptType::SLTIU : {
        value = alu.IsLessThanUnsigned(tmp.value1, tmp.imm);
        break;
      }
      case OptType::XORI : {
        value = alu.XOR(tmp.value1, tmp.imm);
        break;
      }
      case OptType::ORI : {
        value = alu.OR(tmp.value1, tmp.imm);
        break;
      }
      case OptType::ANDI : {
        value = alu.AND(tmp.value1, tmp.imm);
        break;
      }
      case OptType::SLLI : {
        value = alu.ShiftLeftLogical(tmp.value1, tmp.imm);
        break;
      }
      case OptType::SRLI : {
        value = alu.ShiftRightLogical(tmp.value1, tmp.imm);
        break;
      }
      case OptType::SRAI : {
        value = alu.ShiftRightAri(tmp.value1, tmp.imm);
        break;
      }
      case OptType::ADD : {
        value = alu.ADD(tmp.value1, tmp.value2);
        break;
      }
      case OptType::SUB : {
        value = alu.ADD(alu.ADD(tmp.value1, ~tmp.value2), 1);
        break;
      }
      case OptType::SLL : {
        value = alu.ShiftLeftLogical(tmp.value1, tmp.value2);
        break;
      }
      case OptType::SLT : {
        value = alu.IsLessThanSigned(tmp.value1, tmp.value2);
        break;
      }
      case OptType::SLTU : {
        value = alu.IsLessThanUnsigned(tmp.value1, tmp.value2);
        break;
      }
      case OptType::XOR : {
        value = alu.XOR(tmp.value1, tmp.value2);
        break;
      }
      case OptType::SRL : {
        value = alu.ShiftRightLogical(tmp.value1, tmp.value2);
        break;
      }
      case OptType::SRA : {
        value = alu.ShiftRightAri(tmp.value1, tmp.value2);
        break;
      }
      case OptType::OR : {
        value = alu.OR(tmp.value1, tmp.value2);
        break;
      }
      case OptType::AND : {
        value = alu.AND(tmp.value1, tmp.value2);
        break;
      }
      default: throw std::exception();
    }
    cdb.PutOnBus(tmp.label, value);
    RemoveEntry(index);
  }

  /*
   * find an entry without dependency
   * if a ST is at top and without dependency,
   *     calculate the addr and value, pop it into lsb and remove entry
   * if a LD is without dependency and has no STs before it,
   *     calculate its addr, pop it into lsb(and then lsb.execute) and remove entry
   */
  void LsExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, LoadStoreBuffer &lsb) {
    if (size_now == 0) return;
    int addr = 0;
    // ST at top prepared?
    if (rss_now[0].opt == OptType::SB || rss_now[0].opt == OptType::SH || rss_now[0].opt == OptType::SW) {
      if (rss_now[0].dependency1 == -1 && rss_now[0].dependency2 == -1) {
        addr = alu.ADD(rss_now[0].value1, rss_now[0].imm);
        lsb.Execute(rss_now[0].opt, addr, rss_now[0].value2, rss_now[0].label, cdb);
        RemoveEntry(0);
        return;
      }
    }
    // LD without STs before prepared?
    for (int i = 0; i< size_now; ++i) {
      if (rss_now[i].opt == OptType::SB || rss_now[i].opt == OptType::SH || rss_now[i].opt == OptType::SW)
        return;
      if (rss_now[i].dependency1 == -1 && rss_now[i].dependency2 == -1) {
        addr = alu.ADD(rss_now[i].value1, rss_now[i].imm);
        lsb.Execute(rss_now[i].opt, addr, 0, rss_now[i].label, cdb);
        RemoveEntry(i);
        return;
      }
    }
  }

private:
  // 从下标0开始放，到size - 1为止
  RssEntry rss_now[RSSSIZE];
  int size_now;
  RssEntry rss_next[RSSSIZE];
  int size_next;

  int FindIndependentEntry() {
    for (int i = 0; i < size_now; ++i) {
      if (rss_now[i].dependency1 == -1 && rss_now[i].dependency2 == -1)
        return i;
    }
    return -1;
  }

  void RemoveEntry(int index) {
    --size_next;
    for (int i = index; i < size_next; ++i) {
      rss_next[i] = rss_next[i + 1];
    }
  }

};

#endif //RISCV_SIMULATOR_RSS_H
