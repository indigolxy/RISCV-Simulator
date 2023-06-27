
#ifndef RISCV_SIMULATOR_CPU_H
#define RISCV_SIMULATOR_CPU_H

#include "../units/RoB.h"
#include "../storage/memory.h"
#include "../units/rss.h"

class CPU {
public:
  CPU() = default;

  void Init() {
    pc = memory.InitInstructions();
  }

  int run();

private:
  class ArithmeticLogicUnit alu;
  class ReorderBuffer rob;
  class InstructionUnit iu;
  class Register reg;
  class LoadStoreBuffer lsb;
  class Memory memory;
  class ReservationStation ls_rss, ari_rss;
  class Predictor;
  class CommonDataBus ready_bus, commit_bus;
  int pc = 0;
  int clk = 0;

  void ExecuteRss();

  void TryIssue();

};

#endif //RISCV_SIMULATOR_CPU_H
