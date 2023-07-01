
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

    friend std::ostream &operator<<(std::ostream &os, const ReservationStation::RssEntry &obj) {
      os << "label = " << obj.label << ", opt = ";
      switch (obj.opt) {
        case OptType::LUI : os << "LUI"; break;
        case OptType::AUIPC : os << "AUIPC"; break;
        case OptType::JAL : os << "JAL"; break;
        case OptType::JALR : os << "JALR"; break;
        case OptType::BEQ : os << "BEQ"; break;
        case OptType::BNE : os << "BNE"; break;
        case OptType::BLT : os << "BLT"; break;
        case OptType::BGE : os << "BGE"; break;
        case OptType::BLTU : os << "BLTU"; break;
        case OptType::BGEU : os << "BGEU"; break;
        case OptType::LB : os << "LB"; break;
        case OptType::LH : os << "LH"; break;
        case OptType::LW : os << "LW"; break;
        case OptType::LBU : os << "LBU"; break;
        case OptType::LHU : os << "LHU"; break;
        case OptType::SB : os << "SB"; break;
        case OptType::SH : os << "SH"; break;
        case OptType::SW : os << "SW"; break;
        case OptType::ADDI : os << "ADDI"; break;
        case OptType::SLTI : os << "SLTI"; break;
        case OptType::SLTIU : os << "SLTIU"; break;
        case OptType::XORI : os << "XORI"; break;
        case OptType::ORI : os << "ORI"; break;
        case OptType::ANDI : os << "ANDI"; break;
        case OptType::SLLI : os << "SLLI"; break;
        case OptType::SRLI : os << "SRLI"; break;
        case OptType::SRAI : os << "SRAI"; break;
        case OptType::ADD : os << "ADD"; break;
        case OptType::SUB : os << "SUB"; break;
        case OptType::SLL : os << "SLL"; break;
        case OptType::SLT : os << "SLT"; break;
        case OptType::SLTU : os << "SLTU"; break;
        case OptType::XOR : os << "XOR"; break;
        case OptType::SRL : os << "SRL"; break;
        case OptType::SRA : os << "SRA"; break;
        case OptType::OR : os << "OR"; break;
        case OptType::AND : os << "AND"; break;
      }
      os << std::hex << ", value1 = " << obj.value1 << std::dec << ", dependency1 = " << obj.dependency1;
      os << std::hex <<  ", value2 = " << obj.value2 << std::dec << ", dependency2 = " << obj.dependency2;
      os << ", imm = " << obj.imm;
      return os;
    }
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

  void Clear() {size_next = 0;}

  /*
   * read values and dependency in reg
   * add an entry
   */
  void issue(int rob_index, const InstructionUnit::Instruction &ins, const Register &reg, int pc);

  /*
   * find an entry without dependency and calculate in ALU and get result
   * put the information into bus(label, value)
   * remove entry
   */
  void AriExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, int pc);

  /*
   * find an entry without dependency
   * if a ST is at top and without dependency,
   *     calculate the addr and value, pop it into lsb and remove entry
   * if a LD is without dependency and has no STs before it,
   *     calculate its addr, pop it into lsb(and then lsb.execute) and remove entry
   */
  void LsExecute(const ArithmeticLogicUnit &alu, CommonDataBus &cdb, LoadStoreBuffer &lsb);

  /*
   * monitor bus and clear dependency(check dependency)
   */
  void CheckBus(const CommonDataBus &cdb1, const CommonDataBus &cdb2);

  void print();

private:
  // 从下标0开始放，到size - 1为止
  RssEntry rss_now[RSSSIZE];
  int size_now = 0;
  RssEntry rss_next[RSSSIZE];
  int size_next = 0;

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
