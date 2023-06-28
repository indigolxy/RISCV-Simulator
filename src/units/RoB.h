
#ifndef RISCV_SIMULATOR_ROB_H
#define RISCV_SIMULATOR_ROB_H

#include "../utils/circular_queue.h"
#include "instuction.h"
#include "register.h"
#include "rss.h"

class ReorderBuffer {
private:
  struct RoBEntry {
    int pc;
    int label;
    OptType opt;
    bool ready = false;
    int rd = -1; // opt == ADDI && rd == -1 represents END
                 // opt ==
    int value = 0;
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
    CircularQueue<RoBEntry, ROBSIZE>::iterator iter = rob_now.front();
    if (!iter->ready) return {0, false}; // nothing to commit

    // .END
    if (iter->opt == OptType::ADDI && iter->rd == -1) {
      return {1, reg.GetRet()};
    }

    // ST: put on bus, lsb will receive call and start store
    // can remove the entry immediately
    if (iter->opt == OptType::SB || iter->opt == OptType::SH || iter->opt == OptType::SW) {
      cdb.PutOnBus(iter->label, 0, 0); // only need label
    }
    // for AUIPC and JAL: value need to be calculated with pc
    else if (iter->opt == OptType::AUIPC || iter->opt == OptType::JAL) {
      cdb.PutOnBus(iter->label, iter->pc + iter->value, iter->rd);
    }
    // for B-type: need to check pc prediction: if false, clear pipeline; else, do nothing
    else if (iter->opt == OptType::BEQ || iter->opt == OptType::BNE || iter->opt == OptType::BLT || iter->opt == OptType::BGE || iter->opt == OptType::BLTU || iter->opt == OptType::BGEU) {
      int ans_pc = iter->pc + iter->value;
      ++iter;
      if (iter->pc != ans_pc) {
        return {2, ans_pc};
      }
    }
    // for JALR: put pc + 4 on bus, send to reg. check pc prediction
    else if (iter->opt == OptType::JALR) {
      cdb.PutOnBus(iter->label, iter->pc + 4, iter->rd);
      int ans_pc = iter->value;
      ++iter;
      if (iter->pc != ans_pc) {
        return {2, ans_pc};
      }
    }
    else {
      cdb.PutOnBus(iter->label, iter->value, iter->rd);
    }
    rob_next.pop();
    return {0, 0};
  }

  void Clear() {
    rob_next.clear();
  }

  void CheckBus(const CommonDataBus &cdb) {
    CircularQueue<RoBEntry, ROBSIZE>::iterator iter = rob_now.front();
    while (iter != rob_now.end()) {
      std::pair<bool, int> tmp = cdb.TryGetValue(iter->label);
      if (tmp.first) {
        CircularQueue<RoBEntry, ROBSIZE>::iterator iter_next = rob_next.find(iter->label);
        iter_next->ready = true;
        iter_next->value = tmp.second;
      }
      ++iter;
    }
  }

private:
  CircularQueue<RoBEntry, ROBSIZE> rob_now;
  CircularQueue<RoBEntry, ROBSIZE> rob_next;
};

#endif //RISCV_SIMULATOR_ROB_H
