
#ifndef RISCV_SIMULATOR_ROB_H
#define RISCV_SIMULATOR_ROB_H

#include "../utils/circular_queue.h"
#include "instuction.h"
#include "register.h"
#include "rss.h"

class ReorderBuffer {
private:
  struct RoBEntry {
    OptType opt;
    bool ready = false;
    int destination_register = -1;
    int value = 0;
  };

public:
  ReorderBuffer() = default;

  void flush() {rob_now = rob_next;}

  bool full() {return rob_now.full();}

  /*
   * add an entry in rob, update rd's dependency in register
   */
  int issue(const InstructionUnit::Instruction &ins, Register &reg) {
    RoBEntry tmp;
    tmp.opt = ins.opt;
    if (ins.type != InstructionType::S && ins.type != InstructionType::B) {
      tmp.destination_register = ins.rd;
    }
    int index = rob_next.push(tmp);
    if (ins.type == InstructionType::U || ins.type == InstructionType::J || ins.type == InstructionType::I || ins.type == InstructionType::R) {
      reg.SetDependency(ins.rd, index);
    }
    return index;
  }

private:
  CircularQueue<ReorderBuffer::RoBEntry, ROBSIZE> rob_now;
  CircularQueue<ReorderBuffer::RoBEntry, ROBSIZE> rob_next;
};

#endif //RISCV_SIMULATOR_ROB_H
