
#ifndef RISCV_SIMULATOR_ROB_H
#define RISCV_SIMULATOR_ROB_H

#include "../utils/circular_queue.h"
#include "instuction.h"
#include "register.h"
#include "rss.h"

class ReorderBuffer {
private:
  struct RoBEntry {
    int pc = -1;
    int label = -1;
    OptType opt;
    bool ready = false;
    int rd = -1; // opt == ADDI && rd == -1 represents END
                 // opt ==
    int value = 0;

    friend std::ostream &operator<<(std::ostream &os, const ReorderBuffer::RoBEntry &obj) {
      os << "label = " << std::dec << obj.label << ", pc = " << std::hex << obj.pc << std::dec << ", opt = ";
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
      os << ", rd = " << obj.rd << ", value = " << obj.value;
      if (obj.ready) os << ", is ready.";
      else os << ", is not ready.";
      return os;
    }
  };

public:
  ReorderBuffer() = default;

  void flush() {rob_now = rob_next;}

  bool full() {return rob_now.full();}

  /*
   * add an entry in rob, update rd's dependency in register
   */
  int issue(const InstructionUnit::Instruction &ins, Register &reg, int pc) {
    RoBEntry tmp;
    tmp.pc = pc;
    tmp.opt = ins.opt;
    if (ins.type != InstructionType::S && ins.type != InstructionType::B) {
      tmp.rd = ins.rd;
    }
    if (ins.opt == OptType::ADDI && ins.rd == 10 && ins.imm == 255 && ins.rs1 == 0) {
      tmp.rd = -1;
    }
    int index = rob_next.push(tmp);
    rob_next.back()->label = index;
    if (ins.type == InstructionType::U || ins.type == InstructionType::J || ins.type == InstructionType::I || ins.type == InstructionType::R) {
      reg.SetDependency(ins.rd, index);
    }
    return index;
  }

  /*
   *  check entry at front, if ready, commit, put on commit bus, pop
   *                        else return
   *
   *  ST: put on bus, lsb will start store, remove entry immediately
   *  AUIPC and JAL: calculate value with pc
   *  B-type: check pc prediction
   *  JALR: put on bus and check pc prediction
   *  others: just put on bus
   *
   *  .END: return {1, a0}
   *  prediction failed: return {2, correct_pc}
   *  else return {0, 0}
   */
  std::pair<int, int> Commit(CommonDataBus &cdb, const Register &reg) {
    if (rob_now.empty()) return {0, 0};
    CircularQueue<RoBEntry, ROBSIZE>::iterator iter = rob_now.front();
    if (!iter->ready) return {0, false}; // nothing to commit

    // .END
    if (iter->opt == OptType::ADDI && iter->rd == -1) {
      return {1, reg.GetRet()};
    }

    // ST: put on bus, lsb will receive call and start store
    // can remove the entry immediately
    if (iter->opt == OptType::SB || iter->opt == OptType::SH || iter->opt == OptType::SW) {
      std::cout << std::hex << "commit: pc = " << iter->pc << std::dec << std::endl;
//      reg.print();

      cdb.PutOnBus(iter->label, 0, 0); // only need label
    }
    // for AUIPC and JAL: value need to be calculated with pc
    else if (iter->opt == OptType::AUIPC || iter->opt == OptType::JAL) {
      std::cout << std::hex << "commit: pc = " << iter->pc << std::dec << std::endl;
//      reg.print();

      cdb.PutOnBus(iter->label, iter->value, iter->rd);
    }
    // for B-type: need to check pc prediction: if false, clear pipeline; else, do nothing
    else if (iter->opt == OptType::BEQ || iter->opt == OptType::BNE || iter->opt == OptType::BLT || iter->opt == OptType::BGE || iter->opt == OptType::BLTU || iter->opt == OptType::BGEU) {
      int ans_pc = iter->pc + iter->value;
      std::cout << std::hex << "commit: pc = " << iter->pc << std::dec << std::endl;
//      reg.print();

      ++iter;
      if (iter->pc != ans_pc) {
        rob_next.pop();
        return {2, ans_pc};
      }
    }
    // for JALR: put pc + 4 on bus, send to reg. check pc prediction
    else if (iter->opt == OptType::JALR) {
      std::cout << std::hex << "commit: pc = " << iter->pc << std::dec << std::endl;
//      reg.print();

      cdb.PutOnBus(iter->label, iter->pc + 4, iter->rd);
      int ans_pc = iter->value;
      ++iter;
      if (iter->pc != ans_pc) {
        rob_next.pop();
        return {2, ans_pc};
      }
    }
    else {
      std::cout << std::hex << "commit: pc = " << iter->pc << std::dec << std::endl;
//      reg.print();

      cdb.PutOnBus(iter->label, iter->value, iter->rd);
    }
    rob_next.pop();
    return {0, 0};
  }

  void Clear() {
    rob_next.clear();
  }

  void CheckBus(const CommonDataBus &cdb) {
    if (rob_next.empty()) return;
    CircularQueue<RoBEntry, ROBSIZE>::iterator iter = rob_next.front();
    while (iter != rob_next.end()) {
      std::pair<bool, int> tmp = cdb.TryGetValue(iter->label);
      if (tmp.first) {
//        CircularQueue<RoBEntry, ROBSIZE>::iterator iter_next = rob_next.find(iter->label);
        iter->ready = true;
        iter->value = tmp.second;
      }
      ++iter;
    }
  }

  void Print() {
    std::cout << "----------------ROB_NOW--------------------" << std::endl;
    rob_now.print();
    std::cout << "----------------ROB_NEXT--------------------" << std::endl;
    rob_next.print();
  }

private:
  CircularQueue<RoBEntry, ROBSIZE> rob_now;
  CircularQueue<RoBEntry, ROBSIZE> rob_next;
};

#endif //RISCV_SIMULATOR_ROB_H
