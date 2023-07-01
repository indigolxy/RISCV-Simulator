
#ifndef RISCV_SIMULATOR_CPU_H
#define RISCV_SIMULATOR_CPU_H

#include "../units/rob.h"
#include "../storage/memory.h"
#include "../units/rss.h"

class CPU {
public:
  CPU() = default;

  void Init() {
    pc = mem.InitInstructions();
  }

  u8 run();

private:
  class ArithmeticLogicUnit alu;
  class ReorderBuffer rob;
  class InstructionUnit iu;
  class Register reg;
  class LoadStoreBuffer lsb;
  class Memory mem;
  class ReservationStation ls_rss, ari_rss;
  class Predictor predictor;
  class CommonDataBus ready_bus, commit_bus;
  int pc = 0;
  bool pc_start = true;
  int jump_pc = -1;
  int clk = 0;

  void ClearPipeline();

  void ExecuteRss();

  void TryIssue();

  void AccessMem();

  std::pair<u8, bool> TryCommit();

  void CheckBus();

  void Flush();

};

#endif //RISCV_SIMULATOR_CPU_H
